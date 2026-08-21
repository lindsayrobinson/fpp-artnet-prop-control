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
// Each prop has an independent Art-Net colour mode:
//   mode 0-127   = FULL SEQUENCE COLOUR. Keep the sequence RGB values and
//                  ignore that prop's Art-Net RGB sliders while playing.
//   mode 128-255 = DESK COLOUR OVERRIDE. Keep the sequence per-pixel
//                  intensity/pattern but recolour it with Art-Net RGB.
//
// When FPP is idle, both modes output the desk-selected solid RGB colour so
// the props can be used directly from the lighting console with no sequence.
//
// In all cases the per-prop dimmer is applied after colour selection and the
// global master dimmer is ALWAYS the final operation.
//
// Art-Net slots are expected to be mapped consecutively into FPP starting at
// APCControlBaseChannel (default FPP channel 10001):
//   1  Master
//   2  Letters dimmer
//   3  Letters red
//   4  Letters green
//   5  Letters blue
//   6  Letters colour mode
//   7-9 spare
//   10 Festoon dimmer
//   11 Festoon red
//   12 Festoon green
//   13 Festoon blue
//   14 Festoon colour mode

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
        const uint16_t lettersMode = lettersMode_.load(std::memory_order_relaxed);

        const uint16_t festoonDim = festoonDim_.load(std::memory_order_relaxed);
        const uint16_t festoonR   = festoonR_.load(std::memory_order_relaxed);
        const uint16_t festoonG   = festoonG_.load(std::memory_order_relaxed);
        const uint16_t festoonB   = festoonB_.load(std::memory_order_relaxed);
        const uint16_t festoonMode = festoonMode_.load(std::memory_order_relaxed);

        const bool sequencePlaying = Player::INSTANCE.IsPlaying();

        processGroup(data,
                     lettersStartChannel_.load(std::memory_order_relaxed),
                     lettersPixels_.load(std::memory_order_relaxed),
                     lettersColorOrder_.load(std::memory_order_relaxed),
                     lettersR, lettersG, lettersB, lettersMode,
                     lettersDim, master, sequencePlaying);

        processGroup(data,
                     festoonStartChannel_.load(std::memory_order_relaxed),
                     festoonPixels_.load(std::memory_order_relaxed),
                     festoonColorOrder_.load(std::memory_order_relaxed),
                     festoonR, festoonG, festoonB, festoonMode,
                     festoonDim, master, sequencePlaying);
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
    std::atomic<int> controlBaseChannel_{10001};

    // Latched live control values. Neutral defaults make startup safe until the
    // first Art-Net frame is merged and captured.
    std::atomic<uint16_t> master_{255};
    std::atomic<uint16_t> lettersDim_{255};
    std::atomic<uint16_t> lettersR_{255};
    std::atomic<uint16_t> lettersG_{255};
    std::atomic<uint16_t> lettersB_{255};
    std::atomic<uint16_t> lettersMode_{255};
    std::atomic<uint16_t> festoonDim_{255};
    std::atomic<uint16_t> festoonR_{255};
    std::atomic<uint16_t> festoonG_{255};
    std::atomic<uint16_t> festoonB_{255};
    std::atomic<uint16_t> festoonMode_{255};

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
        lettersMode_.store(data[controlBase0 + 5], std::memory_order_relaxed);
        festoonDim_.store(data[controlBase0 + 9], std::memory_order_relaxed);
        festoonR_.store(data[controlBase0 + 10], std::memory_order_relaxed);
        festoonG_.store(data[controlBase0 + 11], std::memory_order_relaxed);
        festoonB_.store(data[controlBase0 + 12], std::memory_order_relaxed);
        festoonMode_.store(data[controlBase0 + 13], std::memory_order_relaxed);
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
                             uint16_t colorMode,
                             uint16_t localDimmer,
                             uint16_t master,
                             bool sequencePlaying) {
        if (pixelCount <= 0) {
            return;
        }

        const int start0 = startChannel1 - 1;
        const auto offsets = colorOffsets(colorOrder);
        const bool fullSequenceColour = sequencePlaying && colorMode < 128;

        for (int pixel = 0; pixel < pixelCount; ++pixel) {
            const int ch = start0 + (pixel * 3);

            uint16_t r;
            uint16_t g;
            uint16_t b;

            if (fullSequenceColour) {
                // FULL SEQUENCE COLOUR MODE: preserve the xLights/FPP RGB values
                // exactly and ignore the Art-Net RGB colour sliders.
                r = data[ch + offsets[0]];
                g = data[ch + offsets[1]];
                b = data[ch + offsets[2]];
            } else if (sequencePlaying) {
                // DESK COLOUR OVERRIDE MODE: preserve only the sequence pixel's
                // intensity/pattern, then paint it with the current Art-Net RGB.
                const uint16_t seqR = data[ch + offsets[0]];
                const uint16_t seqG = data[ch + offsets[1]];
                const uint16_t seqB = data[ch + offsets[2]];
                const uint16_t patternLevel = std::max({seqR, seqG, seqB});

                r = scale8(redLevel,   patternLevel);
                g = scale8(greenLevel, patternLevel);
                b = scale8(blueLevel,  patternLevel);
            } else {
                // IDLE: there is no sequence colour/pattern to preserve, so the
                // desk directly supplies a solid colour across the entire prop.
                r = redLevel;
                g = greenLevel;
                b = blueLevel;
            }

            // Per-prop dimmer always applies in both colour modes.
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
        // Fourteen consecutive control channels must fit in the FPP buffer.
        int controlBase = settingInt("APCControlBaseChannel", 10001);
        controlBase = std::clamp(controlBase, 1, kMaxChannels - 13);
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
