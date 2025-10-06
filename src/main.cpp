#include <gstreamer-1.0/gst/gst.h>
#include <iostream>

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
    std::cout << pipeline << "\n";
    srtsrc = gst_element_factory_make("srtsrc", "srtsrc");
    std::cout << srtsrc << "\n";
    decodebin = gst_element_factory_make("decodebin", "decodebin");
    std::cout << decodebin << "\n";
    videoconvert = gst_element_factory_make("videoconvert", "videoconvert");
    std::cout << videoconvert << "\n";
    ndisink = gst_element_factory_make("ndisink", "ndisink");
    std::cout << ndisink << "\n";

    if (!pipeline || !srtsrc || !decodebin || !videoconvert || !ndisink) {
        g_printerr("One element could not be created. Exiting.\n");
        return -1;
    }

    g_object_set(srtsrc, "uri", "srt://https://skylog-ar.broadteam.eu/multiview", NULL);

    gst_bin_add_many(GST_BIN(pipeline), srtsrc, decodebin, videoconvert, ndisink, NULL);

    if (!gst_element_link_many(srtsrc, decodebin, videoconvert, ndisink, NULL)) {
        g_printerr("Elements could not be linked.\n");
        gst_object_unref(pipeline);
        return -1;
    }

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
