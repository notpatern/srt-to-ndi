#include <gstreamer-1.0/gst/gst.h>

static void on_pad_added(GstElement *element, GstPad *pad, gpointer data) {
    GstPad *sinkpad = (GstPad *)data;
    if (gst_pad_is_linked(sinkpad)) {
        g_print("Pad already linked\n");
        return;
    }
    GstPadLinkReturn ret = gst_pad_link(pad, sinkpad);
    if (ret != GST_PAD_LINK_OK) {
        g_printerr("Failed to link pads\n");
    }
}

static gboolean bus_call(GstBus *bus, GstMessage *msg, gpointer data) {
    GMainLoop *loop = (GMainLoop *)data;
    switch(GST_MESSAGE_TYPE(msg)) {
        case GST_MESSAGE_EOS:
            g_print("End of stream\n");
            g_main_loop_quit(loop);
            break;
        case GST_MESSAGE_ERROR: {
            gchar *debug;
            GError *error;
            gst_message_parse_error(msg, &error, &debug);
            g_printerr("ERROR: %s\n", error->message);
            g_free(debug);
            g_error_free(error);
            g_main_loop_quit(loop);
            break;
        }
        default:
            break;
    }
    return TRUE;
}

int main(int argc, char *argv[]) {
    GstElement *pipeline, *srtsrc, *decodebin, *videoconvert, *ndisink;
    GstBus *bus;
    GMainLoop *main_loop;

    gst_init(&argc, &argv);

    pipeline = gst_pipeline_new("pipeline");
    decodebin = gst_element_factory_make("decodebin", "decodebin");
    videoconvert = gst_element_factory_make("videoconvert", "videoconvert");
    ndisink = gst_element_factory_make("ndisink", "ndisink");
    srtsrc = gst_element_factory_make("srtsrc", "srtsrc");

    if (!pipeline) {
        g_printerr("Pipeline could not be created. Exiting.\n");
        return -1;
    }
    if (!decodebin) {
        g_printerr("decodebin could not be created. Exiting.\n");
        return -1;
    }
    if (!videoconvert) {
        g_printerr("videoconvert could not be created. Exiting.\n");
        return -1;
    }
    if (!ndisink) {
        g_printerr("ndisink could not be created. Exiting.\n");
        return -1;
    }
    if (!srtsrc) {
        return -1;
    }

    g_object_set(srtsrc, "uri", "srt://192.168.104.99:60002", NULL);

    // Add all elements to the pipeline
    gst_bin_add_many(GST_BIN(pipeline), srtsrc, decodebin, videoconvert, ndisink, NULL);

    // Link srtsrc to decodebin
    if (!gst_element_link(srtsrc, decodebin)) {
        g_printerr("Failed to link srtsrc to decodebin\n");
        gst_object_unref(pipeline);
        return -1;
    }

    // Link videoconvert to ndisink
    if (!gst_element_link(videoconvert, ndisink)) {
        g_printerr("Failed to link videoconvert to ndisink\n");
        gst_object_unref(pipeline);
        return -1;
    }

    // Connect the "pad-added" signal for decodebin
    g_signal_connect(decodebin, "pad-added", G_CALLBACK(on_pad_added), gst_element_get_static_pad(videoconvert, "sink"));

    bus = gst_pipeline_get_bus(GST_PIPELINE(pipeline));
    gst_bus_add_watch(bus, bus_call, &main_loop);
    gst_object_unref(bus);

    GstStateChangeReturn ret = gst_element_set_state(pipeline, GST_STATE_PLAYING);
    if (ret == GST_STATE_CHANGE_FAILURE) {
        g_printerr("Pipeline failed to enter PLAYING state.\n");
        gst_object_unref(pipeline);
        return -1;
    }

    main_loop = g_main_loop_new(NULL, FALSE);
    g_main_loop_run(main_loop);

    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(pipeline);
    g_main_loop_unref(main_loop);

    return 0;
}

