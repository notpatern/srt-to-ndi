#include <gst/gst.h>
#include <iostream>

int main(int argc, char *argv[]) {
    GstElement *pipeline;
    GstBus *bus;
    GstMessage *msg;

    gst_init(&argc, &argv);

    const char *srt_uri = "srt://192.168.104.99:60002";

    pipeline = gst_parse_launch(
        "srtserversrc uri=srt://127.0.0.1:8888?mode=listener ! "
        "tsdemux ! h264parse ! avdec_h264 ! "
        "videoconvert ! "
        "ndisinkcombiner name=ndi ! ndisink ndi-name=\"SRT to NDI\" "
        "tsdemux0. ! queue ! aacparse ! avdec_aac ! "
        "audioconvert ! audioresample ! ndi.",
        NULL
    );

    if (!pipeline) {
        std::cerr << "Failed to create pipeline" << std::endl;
        return -1;
    }

    std::cout << "Starting SRT to NDI stream..." << std::endl;
    std::cout << "SRT URI: " << srt_uri << std::endl;
    std::cout << "NDI Name: SRT to NDI" << std::endl;

    gst_element_set_state(pipeline, GST_STATE_PLAYING);

    bus = gst_element_get_bus(pipeline);
    msg = gst_bus_timed_pop_filtered(
        bus,
        GST_CLOCK_TIME_NONE,
        (GstMessageType)(GST_MESSAGE_ERROR | GST_MESSAGE_EOS)
    );

    if (msg != NULL) {
        GError *err;
        gchar *debug_info;

        switch (GST_MESSAGE_TYPE(msg)) {
            case GST_MESSAGE_ERROR:
                gst_message_parse_error(msg, &err, &debug_info);
                std::cerr << "Error: " << err->message << std::endl;
                std::cerr << "Debug: " << (debug_info ? debug_info : "none") << std::endl;
                g_clear_error(&err);
                g_free(debug_info);
                break;
            case GST_MESSAGE_EOS:
                std::cout << "End-Of-Stream reached" << std::endl;
                break;
            default:
                break;
        }
        gst_message_unref(msg);
    }

    gst_object_unref(bus);
    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(pipeline);

    return 0;
}
