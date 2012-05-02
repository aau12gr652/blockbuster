#include <postoffice/Postoffice.h>
#include <serializer/serializer.hpp>
#include <vector>
#include <utility>
#include <stdlib.h>
#include <cstring>
#include <boost/signals2.hpp>

class blockbuster : public PostOffice
{
private:
    serializer m_serializer;
    std::vector<uint32_t> serialized_buffer_table;

    uint32_t layers;
    uint32_t field_size;
    uint32_t generation_size;
    uint32_t symbol_size;
    std::vector<uint32_t> layer_sizes;

    void serialize_avpacket(AVPacket* pkt); // Takes an avpacket and treats it with care and respect, before serializing it with a bunch of other avpackets.
    void make_avpacket(AVPacket* pkt);

public:

    boost::signals2::signal<AVPacket*> signal_new_avpacket;

    prepare_for_kodo_encoder(AVPacket*);


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



};
