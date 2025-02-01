/*
 * Copyright (C) 2022 The LineageOS Project
 * Copyright (C) 2025 Neoteric OS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define LOG_TAG "UdfpsHandler.cmi"

#include <android-base/logging.h>
#include <android-base/unique_fd.h>
#include <fcntl.h>
#include <fstream>
#include <inttypes.h>
#include <poll.h>
#include <thread>
#include <unistd.h>

#include "UdfpsHandler.h"

// Fingerprint hwmodule commands
#define COMMAND_NIT 10
#define PARAM_NIT_UDFPS 1
#define PARAM_NIT_NONE 0

#define FOD_STATUS_ON 1
#define FOD_STATUS_OFF -1

#define TOUCH_DEV_PATH "/dev/xiaomi-touch"
#define Touch_Fod_Enable 10
#define TOUCH_MAGIC 0x5400
#define TOUCH_IOC_SETMODE TOUCH_MAGIC + 0
#define DISPPARAM_FOD_HBM_OFF "0xE0000"

#define DISP_PARAM_PATH "/sys/devices/platform/soc/ae00000.qcom,mdss_mdp/drm/card0/card0-DSI-1/disp_param"
#define FOD_UI_PATH "/sys/devices/platform/soc/soc:qcom,dsi-display-primary/fod_ui"

template <typename T>
static void set(const std::string& path, const T& value) {
    std::ofstream file(path);
    file << value;
}

static bool readBool(int fd) {
    char c;
    int rc = lseek(fd, 0, SEEK_SET);
    if (rc) {
        LOG(ERROR) << "failed to seek fd, err: " << rc;
        return false;
    }

    rc = read(fd, &c, sizeof(char));
    if (rc != 1) {
        LOG(ERROR) << "failed to read bool from fd, err: " << rc;
        return false;
    }

    return c != '0';
}

class XiaomiUdfpsHandler : public UdfpsHandler {
public:
    void init(fingerprint_device_t* device) {
        mDevice = device;
        touch_fd_ = android::base::unique_fd(open(TOUCH_DEV_PATH, O_RDWR));

        std::thread([this]() {
            int fd = open(FOD_UI_PATH, O_RDONLY);
            if (fd < 0) {
                LOG(ERROR) << "failed to open fd, err: " << fd;
                return;
            }

            struct pollfd fodUiPoll = {
                .fd = fd,
                .events = POLLERR | POLLPRI,
                .revents = 0,
            };

            while (true) {
                int rc = poll(&fodUiPoll, 1, -1);
                if (rc < 0) {
                    LOG(ERROR) << "failed to poll fd, err: " << rc;
                    continue;
                }

                bool fingerDown = readBool(fd);
                LOG(ERROR) << "fod_ui status: %d" << fingerDown;
                mDevice->extCmd(mDevice, COMMAND_NIT, readBool(fd) ? PARAM_NIT_UDFPS : PARAM_NIT_NONE);
                if (!fingerDown) {
                    set(DISP_PARAM_PATH, DISPPARAM_FOD_HBM_OFF);
                }
            }
        }).detach();
    }

    void onFingerDown(uint32_t /*x*/, uint32_t /*y*/, float /*minor*/, float /*major*/) {
        int arg[2] = {Touch_Fod_Enable, FOD_STATUS_ON};
        ioctl(touch_fd_.get(), TOUCH_IOC_SETMODE, &arg);
    }

    void onFingerUp() {
        // nothing
    }

    void onAcquired(int32_t result, int32_t vendorCode) {
        if (result == FINGERPRINT_ACQUIRED_GOOD) {
            mDevice->extCmd(mDevice, COMMAND_NIT, PARAM_NIT_NONE);
            set(DISP_PARAM_PATH, DISPPARAM_FOD_HBM_OFF);
        } else if (vendorCode == 21 || vendorCode == 23) {
            /*
             * vendorCode = 21 waiting for fingerprint authentication
             * vendorCode = 23 waiting for fingerprint enroll
             */
            int arg[2] = {Touch_Fod_Enable, FOD_STATUS_ON};
            ioctl(touch_fd_.get(), TOUCH_IOC_SETMODE, &arg);
        }
    }

    void cancel() {
        // nothing
    }

private:
    fingerprint_device_t* mDevice;
    android::base::unique_fd touch_fd_;
};

static UdfpsHandler* create() {
    return new XiaomiUdfpsHandler();
}

static void destroy(UdfpsHandler* handler) {
    delete handler;
}

extern "C" UdfpsHandlerFactory UDFPS_HANDLER_FACTORY = {
    .create = create,
    .destroy = destroy,
};
