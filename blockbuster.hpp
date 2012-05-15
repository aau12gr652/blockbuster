//#include <postoffice/Postoffice.h>
//#include <serializer/serializer.hpp>
//#include <node/kodo_decoder.h>
//#include <server/kodo_encoder.h>

extern "C" {
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
}

#include <server/kodo_encoder.h>
#include <node/kodo_decoder.h>
#include <postoffice/Postoffice.h>
#include <serializer/serializer.hpp>

#include <vector>
#include <utility>
#include <stdlib.h>
#include <cstring>
#include <cmath>
#include <assert.h>
#include <boost/signals2.hpp>
#include <boost/thread.hpp>

class blockbuster
{
private:
    postoffice *benjamin_krebs;
    kodo_encoder *m_kodo_encoder;
    kodo_decoder *m_kodo_decoder;

    serializer *m_serializer;
    std::vector<uint32_t> serialized_buffer_table;

    bool inbound;
    bool mailbox_active;

    boost::thread transmission_thread;
    boost::thread mailboxthread;

    uint32_t layers;
    uint32_t field_size;
    uint32_t generation_size;
    uint32_t symbol_size;
    uint32_t max_packet_size;
    std::vector<uint32_t> layer_sizes;

    void serialize_avpacket(AVPacket* pkt); // Takes an avpacket and treats it with care and respect, before serializing it with a bunch of other avpackets.
    void make_avpacket(uint8_t*, uint32_t);
    void make_layers(uint32_t nb_layers, uint32_t gsize, uint32_t symb_size);

    uint32_t calculate_generation_size_from_gop_size(uint32_t gop_size);
    uint32_t calculate_symbol_size_from_generation_size(uint32_t generation_size);
    uint32_t calculate_layer_size(uint32_t layer_size);

public:

    blockbuster(bool);
    ~blockbuster();
    boost::signals2::signal<void (AVPacket*)> signal_new_avpacket;

    void mailbox_thread();
    void disconnect();
    void connect_to_stream();

    void transmit_generation(uint32_t,uint32_t,float);

    void prepare_for_kodo_encoder(AVPacket*);

    // Needs functionality to take care of layers

    void set_layers(uint32_t);
    uint32_t get_layers();

    void set_layer_size(uint32_t layer_number, uint32_t size);
    uint32_t get_layer_size(uint32_t layer_number);

    void set_field_size(uint32_t q);
    uint32_t get_field_size();

    void set_generation_size(uint32_t);
    uint32_t get_generation_size();

    void set_symbol_size(uint32_t size);
    uint32_t get_symbol_size();

    void set_layer_gamma(uint32_t layer_number, uint32_t percentage);
};
