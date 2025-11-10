#include <gst/gst.h>
#include <iostream>

class SrtToNdiPipeline {
private:
    GstElement *pipeline;
    GstElement *srtsrc;
    GstElement *queue1;
    GstElement *tsdemux;
    GstElement *queue2;
    GstElement *h264parse;
    GstElement *avdec_h264;
    GstElement *videoconvert;
    GstElement *ndisink;
    GstBus *bus;

public:
    SrtToNdiPipeline(const std::string& srt_uri, const std::string& ndi_name) {
        pipeline = gst_pipeline_new("srt-to-ndi");
        srtsrc = gst_element_factory_make("srtsrc", "srt-source");
        queue1 = gst_element_factory_make("queue", "queue1");
        tsdemux = gst_element_factory_make("tsdemux", "demuxer");
        queue2 = gst_element_factory_make("queue", "queue2");
        h264parse = gst_element_factory_make("h264parse", "parser");
        avdec_h264 = gst_element_factory_make("avdec_h264", "decoder");
        videoconvert = gst_element_factory_make("videoconvert", "converter");
        ndisink = gst_element_factory_make("ndisink", "ndi-output");

        if (!pipeline || !srtsrc || !queue1 || !tsdemux || !queue2 ||
            !h264parse || !avdec_h264 || !videoconvert || !ndisink) {
            std::cout << "Not all elements could be created." << std::endl;
            return;
        }
        std::cout << "Elements created" << std::endl;

        g_object_set(G_OBJECT(srtsrc), "uri", srt_uri.c_str(), NULL);
        g_object_set(G_OBJECT(ndisink), "ndi-name", ndi_name.c_str(), NULL);

        std::cout << "Objects set" << std::endl;

        gst_bin_add_many(GST_BIN(pipeline), srtsrc, queue1, tsdemux, queue2, h264parse, avdec_h264, videoconvert, ndisink, NULL);

        std::cout << "bin add many" << std::endl;

        if (!gst_element_link(srtsrc, queue1)) {
            std::cout << "Failed to link srtsrc to queue1" << std::endl;
            return;
        }

        if (!gst_element_link(queue1, tsdemux)) {
            std::cout << "Failed to link queue1 to tsdemux" << std::endl;
            return;
        }

        if (!gst_element_link_many(queue2, h264parse, avdec_h264, videoconvert, ndisink, NULL)) {
            std::cout << "Failed to link decoder pipeline" << std::endl;
            return;
        }

        std::cout << "Nothing failed gamer we gaming so hard" << std::endl;

        g_signal_connect(tsdemux, "pad-added", G_CALLBACK(on_pad_added), queue2);

        bus = gst_element_get_bus(pipeline);
    }

    ~SrtToNdiPipeline() {
        if (bus) {
            gst_object_unref(bus);
        }
        if (pipeline) {
            gst_object_unref(pipeline);
        }
    }

    static void on_pad_added(GstElement *element, GstPad *pad, gpointer data) {
        GstElement *queue = (GstElement *)data;
        GstPad *sinkpad = gst_element_get_static_pad(queue, "sink");

        GstCaps *caps = gst_pad_get_current_caps(pad);
        const GstStructure *str = gst_caps_get_structure(caps, 0);
        const gchar *name = gst_structure_get_name(str);

        if (g_str_has_prefix(name, "video/")) {
            if (!gst_pad_is_linked(sinkpad)) {
                if (gst_pad_link(pad, sinkpad) != GST_PAD_LINK_OK) {
                    std::cout << "Failed to link demuxer pad" << std::endl;
                    return;
                }
            }
        }

        gst_caps_unref(caps);
        gst_object_unref(sinkpad);
    }

    void start() {
        gst_element_set_state(pipeline, GST_STATE_PLAYING);
        std::cout << "Pipeline started..." << std::endl;
    }

    void run() {
        std::cout << "Running" << std::endl;

        if (pipeline) {
            std::cout << "pipeline still active" << std::endl;
        }

        GstMessage *msg = gst_bus_timed_pop_filtered(bus, GST_CLOCK_TIME_NONE, (GstMessageType)(GST_MESSAGE_ERROR | GST_MESSAGE_EOS));

        if (!msg) {
            std::cout << "Message is null" << std::endl;
        }

        if (msg != NULL) {
            GError *err;
            gchar *debug_info;

            switch (GST_MESSAGE_TYPE(msg)) {
                case GST_MESSAGE_ERROR:
                    gst_message_parse_error(msg, &err, &debug_info);
                    std::cout << "Error: " << err->message << std::endl;
                    std::cout << "Debug: " << (debug_info ? debug_info : "none") << std::endl;
                    g_clear_error(&err);
                    g_free(debug_info);
                    break;
                case GST_MESSAGE_EOS:
                    std::cout << "End of stream" << std::endl;
                    break;
                default:
                    break;
            }
            gst_message_unref(msg);
        }

        gst_element_set_state(pipeline, GST_STATE_NULL);
    }
};

int main(int argc, char *argv[]) {
    gst_init(&argc, &argv);

    if (argc < 2) {
        std::cout << "Expected: " << argv[0] << " <srt-uri> [ndi-name]" << std::endl;
        return -1;
    }

    std::string srt_uri = argv[1];
    std::string ndi_name = "SrtToNdi";
    if (argc > 2) {
        ndi_name = argv[2];
    }

    SrtToNdiPipeline pipeline(srt_uri, ndi_name);
    pipeline.start();
    pipeline.run();

    gst_deinit();
    return 0;
}
