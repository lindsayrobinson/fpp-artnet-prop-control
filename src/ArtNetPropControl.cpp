#include <fpp-pch.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <cstdint>
#include <functional>
#include <string>

#include "Plugin.h"
#include "Player.h"
#include "Sequence.h"
#include "log.h"

// FPP 10 native ChannelDataPlugin.
//
// Processing order for each RGB pixel:
//   1. If FPP is playing, derive a brightness mask from the existing sequence
//      pixel using max(R,G,B). This preserves patterns/fades while discarding
//      the sequence hue. If FPP is idle, use a full (255) mask so the desk can
//      light the entire prop without a sequence.
//   2. Generate the pixel colour from the Art-Net R/G/B controls and mask it.
//   3. Apply the per-prop dimmer.
//   4. Apply the global master dimmer (ALWAYS LAST).
//
// The lighting desk therefore owns colour at all times, while an active sequence
// can still own per-pixel pattern/intensity.
//
// Art-Net slots are expected to be mapped consecutively into FPP starting at
// APCControlBaseChannel (default FPP channel 10001):
//   1  Master
//   2  Letters dimmer
//   3  Letters red
//   4  Letters green
//   5  Letters blue
//   6-9 spare
//   10 Festoon dimmer
//   11 Festoon red
//   12 Festoon green
//   13 Festoon blue

class ArtNetPropControlPlugin final : public FPPPlugins::Plugin,
                                      public FPPPlugins::ChannelDataPlugin {
public:
    ArtNetPropControlPlugin()
        : FPPPlugins::Plugin("fpp-artnet-prop-control", true) {
        setDefaultSettings();
        applySettings();
        LogInfo(VB_PLUGIN, "Art-Net Prop Control (FPP 10) loaded\n");
    }

    ~ArtNetPropControlPlugin() override = default;

    // FPP merges live Art-Net/E1.31/DDP bridge data before this callback.
    // Capture the control slots here so sequence playback cannot hide or
    // replace them later in the processing pipeline.
    void modifySequenceData(int /*ms*/, uint8_t* data) override {
        if (data == nullptr || bypass_.load(std::memory_order_relaxed)) {
            return;
        }
        captureControls(data);
    }

    void modifyChannelData(int /*ms*/, uint8_t* data) override {
        if (data == nullptr || bypass_.load(std::memory_order_relaxed)) {
            return;
        }

        // When idle there may not be a sequence-stage callback, so refresh
        // directly from the channel buffer as a fallback. During playback the
        // values below were latched in modifySequenceData() immediately after
        // FPP merged the live bridge input.
        if (!Player::INSTANCE.IsPlaying()) {
            captureControls(data);
        }

        const uint16_t master = master_.load(std::memory_order_relaxed);

        const uint16_t lettersDim = lettersDim_.load(std::memory_order_relaxed);
        const uint16_t lettersR   = lettersR_.load(std::memory_order_relaxed);
        const uint16_t lettersG   = lettersG_.load(std::memory_order_relaxed);
        const uint16_t lettersB   = lettersB_.load(std::memory_order_relaxed);

        const uint16_t festoonDim = festoonDim_.load(std::memory_order_relaxed);
        const uint16_t festoonR   = festoonR_.load(std::memory_order_relaxed);
        const uint16_t festoonG   = festoonG_.load(std::memory_order_relaxed);
        const uint16_t festoonB   = festoonB_.load(std::memory_order_relaxed);

        // When enabled, an active FPP player supplies only per-pixel intensity.
        // When idle, every pixel receives a full mask so Art-Net can light the
        // props with no sequence running.
        const bool useSequenceMask = useSequencePattern_.load(std::memory_order_relaxed) &&
                                     Player::INSTANCE.IsPlaying();

        processGroup(data,
                     lettersStartChannel_.load(std::memory_order_relaxed),
                     lettersPixels_.load(std::memory_order_relaxed),
                     lettersColorOrder_.load(std::memory_order_relaxed),
                     lettersR, lettersG, lettersB,
                     lettersDim, master, useSequenceMask);

        processGroup(data,
                     festoonStartChannel_.load(std::memory_order_relaxed),
                     festoonPixels_.load(std::memory_order_relaxed),
                     festoonColorOrder_.load(std::memory_order_relaxed),
                     festoonR, festoonG, festoonB,
                     festoonDim, master, useSequenceMask);
    }

    std::function<bool()> shutdown() override {
        // No threads, sockets, timers, commands, or API handlers are owned.
        return nullptr;
    }

protected:
    void settingChanged(const std::string& /*key*/, const std::string& /*value*/) override {
        // The FPP monitor has already updated the settings map before this call.
        applySettings();
    }

private:
    static constexpr int kMaxChannels = FPPD_MAX_CHANNELS;

    std::atomic<bool> bypass_{true};
    std::atomic<bool> useSequencePattern_{true};
    std::atomic<int> controlBaseChannel_{10001};

    // Latched live control values. Neutral defaults make startup safe until the
    // first Art-Net frame is merged and captured.
    std::atomic<uint16_t> master_{255};
    std::atomic<uint16_t> lettersDim_{255};
    std::atomic<uint16_t> lettersR_{255};
    std::atomic<uint16_t> lettersG_{255};
    std::atomic<uint16_t> lettersB_{255};
    std::atomic<uint16_t> festoonDim_{255};
    std::atomic<uint16_t> festoonR_{255};
    std::atomic<uint16_t> festoonG_{255};
    std::atomic<uint16_t> festoonB_{255};

    std::atomic<int> lettersStartChannel_{6001};
    std::atomic<int> lettersPixels_{149};
    std::atomic<int> lettersColorOrder_{0};

    std::atomic<int> festoonStartChannel_{1};
    std::atomic<int> festoonPixels_{2000};
    std::atomic<int> festoonColorOrder_{0};

    void captureControls(const uint8_t* data) {
        const int controlBase0 = controlBaseChannel_.load(std::memory_order_relaxed) - 1;

        master_.store(data[controlBase0 + 0], std::memory_order_relaxed);

        lettersDim_.store(data[controlBase0 + 1], std::memory_order_relaxed);
        lettersR_.store(data[controlBase0 + 2], std::memory_order_relaxed);
        lettersG_.store(data[controlBase0 + 3], std::memory_order_relaxed);
        lettersB_.store(data[controlBase0 + 4], std::memory_order_relaxed);

        festoonDim_.store(data[controlBase0 + 9], std::memory_order_relaxed);
        festoonR_.store(data[controlBase0 + 10], std::memory_order_relaxed);
        festoonG_.store(data[controlBase0 + 11], std::memory_order_relaxed);
        festoonB_.store(data[controlBase0 + 12], std::memory_order_relaxed);
    }

    static uint8_t scale8(uint16_t value, uint16_t level) {
        // Rounded 8-bit multiply: 255 leaves the source unchanged.
        return static_cast<uint8_t>((value * level + 127u) / 255u);
    }

    static std::array<int, 3> colorOffsets(int order) {
        // Offsets identify where logical R, G and B live within each 3-byte pixel.
        switch (order) {
        case 1: return {0, 2, 1}; // RBG
        case 2: return {1, 0, 2}; // GRB
        case 3: return {2, 0, 1}; // GBR
        case 4: return {1, 2, 0}; // BRG
        case 5: return {2, 1, 0}; // BGR
        case 0:
        default: return {0, 1, 2}; // RGB
        }
    }

    static void processGroup(uint8_t* data,
                             int startChannel1,
                             int pixelCount,
                             int colorOrder,
                             uint16_t redLevel,
                             uint16_t greenLevel,
                             uint16_t blueLevel,
                             uint16_t localDimmer,
                             uint16_t master,
                             bool useSequenceMask) {
        if (pixelCount <= 0) {
            return;
        }

        const int start0 = startChannel1 - 1;
        const auto offsets = colorOffsets(colorOrder);

        for (int pixel = 0; pixel < pixelCount; ++pixel) {
            const int ch = start0 + (pixel * 3);

            // When a sequence/player is active, preserve only the pixel's
            // intensity. Using max(R,G,B) means fully saturated red, green,
            // blue or white all represent 100% intensity. Sequence black is
            // still black, so chases, twinkles and intentional blackouts work.
            uint16_t patternLevel = 255;
            if (useSequenceMask) {
                const uint16_t seqR = data[ch + offsets[0]];
                const uint16_t seqG = data[ch + offsets[1]];
                const uint16_t seqB = data[ch + offsets[2]];
                patternLevel = std::max({seqR, seqG, seqB});
            }

            // Art-Net owns the actual colour.
            uint16_t r = scale8(redLevel,   patternLevel);
            uint16_t g = scale8(greenLevel, patternLevel);
            uint16_t b = scale8(blueLevel,  patternLevel);

            // Per-prop dimmer comes after colour/pattern.
            r = scale8(r, localDimmer);
            g = scale8(g, localDimmer);
            b = scale8(b, localDimmer);

            // GLOBAL MASTER -- deliberately and explicitly the final step.
            data[ch + offsets[0]] = scale8(r, master);
            data[ch + offsets[1]] = scale8(g, master);
            data[ch + offsets[2]] = scale8(b, master);
        }
    }

    void setDefaultSettings() {
        // Safe in-memory defaults if no plugin config file exists yet.
        if (settings.find("APCBypass") == settings.end()) settings["APCBypass"] = "1";
        if (settings.find("APCUseSequencePattern") == settings.end()) settings["APCUseSequencePattern"] = "1";
        if (settings.find("APCControlBaseChannel") == settings.end()) settings["APCControlBaseChannel"] = "10001";

        if (settings.find("APCLettersStartChannel") == settings.end()) settings["APCLettersStartChannel"] = "6001";
        if (settings.find("APCLettersPixels") == settings.end()) settings["APCLettersPixels"] = "149";
        if (settings.find("APCLettersColorOrder") == settings.end()) settings["APCLettersColorOrder"] = "0";

        if (settings.find("APCFestoonStartChannel") == settings.end()) settings["APCFestoonStartChannel"] = "1";
        if (settings.find("APCFestoonPixels") == settings.end()) settings["APCFestoonPixels"] = "2000";
        if (settings.find("APCFestoonColorOrder") == settings.end()) settings["APCFestoonColorOrder"] = "0";
    }

    int settingInt(const std::string& key, int fallback) const {
        const auto it = settings.find(key);
        if (it == settings.end() || it->second.empty()) {
            return fallback;
        }
        try {
            size_t used = 0;
            const long value = std::stol(it->second, &used, 10);
            if (used != it->second.size() || value < INT32_MIN || value > INT32_MAX) {
                return fallback;
            }
            return static_cast<int>(value);
        } catch (...) {
            return fallback;
        }
    }

    bool settingBool(const std::string& key, bool fallback) const {
        const auto it = settings.find(key);
        if (it == settings.end()) {
            return fallback;
        }
        return it->second == "1" || it->second == "true" ||
               it->second == "on" || it->second == "yes";
    }

    static int clampStart(int value) {
        return std::clamp(value, 1, kMaxChannels);
    }

    static int clampPixels(int requested, int startChannel1) {
        const int availableChannels = kMaxChannels - (startChannel1 - 1);
        const int maxPixels = std::max(0, availableChannels / 3);
        return std::clamp(requested, 0, maxPixels);
    }

    void applySettings() {
        bypass_.store(settingBool("APCBypass", true), std::memory_order_relaxed);
        useSequencePattern_.store(settingBool("APCUseSequencePattern", true), std::memory_order_relaxed);

        // Thirteen consecutive control channels must fit in the FPP buffer.
        int controlBase = settingInt("APCControlBaseChannel", 10001);
        controlBase = std::clamp(controlBase, 1, kMaxChannels - 12);
        controlBaseChannel_.store(controlBase, std::memory_order_relaxed);

        const int lettersStart = clampStart(settingInt("APCLettersStartChannel", 6001));
        const int lettersPixels = clampPixels(settingInt("APCLettersPixels", 149), lettersStart);
        const int lettersOrder = std::clamp(settingInt("APCLettersColorOrder", 0), 0, 5);
        lettersStartChannel_.store(lettersStart, std::memory_order_relaxed);
        lettersPixels_.store(lettersPixels, std::memory_order_relaxed);
        lettersColorOrder_.store(lettersOrder, std::memory_order_relaxed);

        const int festoonStart = clampStart(settingInt("APCFestoonStartChannel", 1));
        const int festoonPixels = clampPixels(settingInt("APCFestoonPixels", 2000), festoonStart);
        const int festoonOrder = std::clamp(settingInt("APCFestoonColorOrder", 0), 0, 5);
        festoonStartChannel_.store(festoonStart, std::memory_order_relaxed);
        festoonPixels_.store(festoonPixels, std::memory_order_relaxed);
        festoonColorOrder_.store(festoonOrder, std::memory_order_relaxed);
    }
};

// FPP 10 can fully unload/reload this plugin at runtime because it leaves no
// threads, callbacks, commands, routes, or other code pointers behind.
FPP_PLUGIN_SUPPORTS_UNLOAD()

extern "C" FPPPlugins::Plugin* createPlugin() {
    return new ArtNetPropControlPlugin();
}
