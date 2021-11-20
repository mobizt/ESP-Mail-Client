#include "OV2640.h"

#define TAG "OV2640"

void OV2640::run(void)
{
    camera_run();
}

int OV2640::getWidth(void)
{
    return camera_get_fb_width();
}

int OV2640::getHeight(void)
{
    return camera_get_fb_height();
}

size_t OV2640::getSize(void)
{
    return camera_get_data_size();
}

uint8_t *OV2640::getfb(void)
{
    return camera_get_fb();
}

camera_framesize_t OV2640::getFrameSize(void)
{
    return _cam_config.frame_size;
}

void OV2640::setFrameSize(camera_framesize_t size)
{
    switch (size)
    {
    case CAMERA_FS_QQVGA:
    case CAMERA_FS_QVGA:
    case CAMERA_FS_VGA:
    case CAMERA_FS_SVGA:
        _cam_config.frame_size = size;
        break;
    default:
        _cam_config.frame_size = CAMERA_FS_SVGA;
        break;
    }
}

camera_pixelformat_t OV2640::getPixelFormat(void)
{
    return _cam_config.pixel_format;
}

void OV2640::setPixelFormat(camera_pixelformat_t format)
{
    switch (format)
    {
    case CAMERA_PF_RGB565:
    case CAMERA_PF_YUV422:
    case CAMERA_PF_GRAYSCALE:
    case CAMERA_PF_JPEG:
        _cam_config.pixel_format = format;
        break;
    default:
        _cam_config.pixel_format = CAMERA_PF_GRAYSCALE;
        break;
    }
}

void OV2640::setVflip(bool enable)
{
    camera_set_vflip(enable);
}


esp_err_t OV2640::init(camera_config_t config)
{
    camera_model_t camera_model;
    memset(&_cam_config, 0, sizeof(_cam_config));
    memcpy(&_cam_config, &config, sizeof(config));

    esp_err_t err = camera_probe(&_cam_config, &camera_model);
    if (err != ESP_OK)
    {
        ESP_LOGD(TAG, "Camera probe failed with error 0x%x", err);
        return err;
    }
    if (camera_model == CAMERA_OV7725)
    {
        ESP_LOGD(TAG, "Detected OV7725 camera");
    }
    else if (camera_model == CAMERA_OV2640)
    {
        ESP_LOGD(TAG,"Detected OV2640 camera");
        _cam_config.jpeg_quality = 15;
    }
    else
    {
        ESP_LOGD(TAG,"Camera not supported");
        return ESP_ERR_CAMERA_NOT_SUPPORTED;
    }
    ESP_ERROR_CHECK(gpio_install_isr_service(0));

    err = camera_init(&_cam_config);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Camera init failed with error 0x%x", err);
        return err;
    }
    
    return ESP_OK;
}
