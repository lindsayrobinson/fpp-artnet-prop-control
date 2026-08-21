#include <fpp-pch.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <cstdint>
#include <functional>
#include <string>

#include "Plugin.h"
#include "Sequence.h"
#include "log.h"

class ArtNetPropControlPlugin : public FPPPlugin {
public:
    ArtNetPropControlPlugin()
        : FPPPlugin("fpp-artnet-prop-control", true) {
        setDefaultSettings();
        applySettings();
        LogInfo(VB_PLUGIN, "Art-Net Prop Control plugin loaded\n");
    }

    ~ArtNetPropControlPlugin() override = default;

    void modifyChannelData(int /*ms*/, uint8_t* data) override {
        if (data == nullptr || bypass_.load(std::memory_order_relaxed)) {
            return;
        }

        const int controlBase1 = controlBaseChannel_.load(std::memory_order_relaxed);
        const int controlBase0 = controlBase1 - 1;

        // Read every control before touching any pixel data. This also avoids
        // changing a control value mid-frame if someone accidentally configures
        // an overlapping range (although overlapping ranges should be avoided).
        const uint16_t master = data[controlBase0 + 0];   // Art-Net slot 1

        const uint16_t lettersDim = data[controlBase0 + 1]; // slot 2
        const uint16_t lettersR   = data[controlBase0 + 2]; // slot 3
        const uint16_t lettersG   = data[controlBase0 + 3]; // slot 4
        const uint16_t lettersB   = data[controlBase0 + 4]; // slot 5

        const uint16_t festoonDim = data[controlBase0 + 9];  // slot 10
        const uint16_t festoonR   = data[controlBase0 + 10]; // slot 11
        const uint16_t festoonG   = data[controlBase0 + 11]; // slot 12
        const uint16_t festoonB   = data[controlBase0 + 12]; // slot 13

        processGroup(data,
                     lettersStartChannel_.load(std::memory_order_relaxed),
                     lettersPixels_.load(std::memory_order_relaxed),
                     lettersColorOrder_.load(std::memory_order_relaxed),
                     lettersR, lettersG, lettersB, lettersDim, master);

        processGroup(data,
                     festoonStartChannel_.load(std::memory_order_relaxed),
                     festoonPixels_.load(std::memory_order_relaxed),
                     festoonColorOrder_.load(std::memory_order_relaxed),
                     festoonR, festoonG, festoonB, festoonDim, master);
    }

#if defined(FPP_PLUGIN_API_VERSION)
    std::function<bool()> shutdown() override {
        // FPP 10+ lifecycle hook. No threads, sockets, timers, or registered
        // callbacks are owned by this plugin, so shutdown is immediate.
        return nullptr;
    }
#endif

protected:
    void settingChanged(const std::string& /*name*/, const std::string& /*value*/) override {
        applySettings();
    }

private:
    // FPP uses a very large global channel buffer; these bounds simply keep a bad
    // setting from indexing outside that buffer.
    static constexpr int kMaxChannels = FPPD_MAX_CHANNELS;

    std::atomic<bool> bypass_{true};
    std::atomic<int> controlBaseChannel_{10001};

    std::atomic<int> lettersStartChannel_{1};
    std::atomic<int> lettersPixels_{149};
    std::atomic<int> lettersColorOrder_{0};

    std::atomic<int> festoonStartChannel_{448};
    std::atomic<int> festoonPixels_{2000};
    std::atomic<int> festoonColorOrder_{0};

    static uint8_t scale8(uint16_t value, uint16_t level) {
        return static_cast<uint8_t>((value * level + 127u) / 255u);
    }

    static std::array<int, 3> colorOffsets(int order) {
        // Returned offsets are R, G, B positions inside the physical 3-channel pixel.
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
                             uint16_t master) {
        if (pixelCount <= 0) {
            return;
        }

        const int start0 = startChannel1 - 1;
        const auto offsets = colorOffsets(colorOrder);

        for (int pixel = 0; pixel < pixelCount; ++pixel) {
            const int ch = start0 + pixel * 3;

            // Step 1: RGB filtering/colour balance.
            uint16_t r = scale8(data[ch + offsets[0]], redLevel);
            uint16_t g = scale8(data[ch + offsets[1]], greenLevel);
            uint16_t b = scale8(data[ch + offsets[2]], blueLevel);

            // Step 2: local prop brightness.
            r = scale8(r, localDimmer);
            g = scale8(g, localDimmer);
            b = scale8(b, localDimmer);

            // Step 3: MASTER DIMMER — deliberately the final operation.
            r = scale8(r, master);
            g = scale8(g, master);
            b = scale8(b, master);

            data[ch + offsets[0]] = static_cast<uint8_t>(r);
            data[ch + offsets[1]] = static_cast<uint8_t>(g);
            data[ch + offsets[2]] = static_cast<uint8_t>(b);
        }
    }

    void setDefaultSettings() {
        // Defaults are in-memory only. settings.json provides the same defaults in
        // the UI; these make the native code safe before any config file exists.
        if (settings.find("APCBypass") == settings.end()) settings["APCBypass"] = "1";
        if (settings.find("APCControlBaseChannel") == settings.end()) settings["APCControlBaseChannel"] = "10001";

        if (settings.find("APCLettersStartChannel") == settings.end()) settings["APCLettersStartChannel"] = "1";
        if (settings.find("APCLettersPixels") == settings.end()) settings["APCLettersPixels"] = "149";
        if (settings.find("APCLettersColorOrder") == settings.end()) settings["APCLettersColorOrder"] = "0";

        if (settings.find("APCFestoonStartChannel") == settings.end()) settings["APCFestoonStartChannel"] = "448";
        if (settings.find("APCFestoonPixels") == settings.end()) settings["APCFestoonPixels"] = "2000";
        if (settings.find("APCFestoonColorOrder") == settings.end()) settings["APCFestoonColorOrder"] = "0";
    }

    int settingInt(const std::string& key, int fallback) const {
        auto it = settings.find(key);
        if (it == settings.end() || it->second.empty()) {
            return fallback;
        }
        try {
            size_t used = 0;
            long value = std::stol(it->second, &used, 10);
            if (used != it->second.size()) {
                return fallback;
            }
            if (value < static_cast<long>(INT32_MIN) || value > static_cast<long>(INT32_MAX)) {
                return fallback;
            }
            return static_cast<int>(value);
        } catch (...) {
            return fallback;
        }
    }

    bool settingBool(const std::string& key, bool fallback) const {
        auto it = settings.find(key);
        if (it == settings.end()) {
            return fallback;
        }
        return it->second == "1" || it->second == "true" || it->second == "on" || it->second == "yes";
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

        // Need 13 consecutive slots beginning here, so keep base+12 in range.
        int controlBase = settingInt("APCControlBaseChannel", 10001);
        controlBase = std::clamp(controlBase, 1, kMaxChannels - 12);
        controlBaseChannel_.store(controlBase, std::memory_order_relaxed);

        int lettersStart = clampStart(settingInt("APCLettersStartChannel", 1));
        int lettersPixels = clampPixels(settingInt("APCLettersPixels", 149), lettersStart);
        int lettersOrder = std::clamp(settingInt("APCLettersColorOrder", 0), 0, 5);
        lettersStartChannel_.store(lettersStart, std::memory_order_relaxed);
        lettersPixels_.store(lettersPixels, std::memory_order_relaxed);
        lettersColorOrder_.store(lettersOrder, std::memory_order_relaxed);

        int festoonStart = clampStart(settingInt("APCFestoonStartChannel", 448));
        int festoonPixels = clampPixels(settingInt("APCFestoonPixels", 2000), festoonStart);
        int festoonOrder = std::clamp(settingInt("APCFestoonColorOrder", 0), 0, 5);
        festoonStartChannel_.store(festoonStart, std::memory_order_relaxed);
        festoonPixels_.store(festoonPixels, std::memory_order_relaxed);
        festoonColorOrder_.store(festoonOrder, std::memory_order_relaxed);
    }
};

#if defined(FPP_PLUGIN_SUPPORTS_UNLOAD)
FPP_PLUGIN_SUPPORTS_UNLOAD()
#endif

extern "C" {
FPPPlugin* createPlugin() {
    return new ArtNetPropControlPlugin();
}
}
