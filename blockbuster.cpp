#include "blockbuster.hpp"

blockbuster::blockbuster()
{
    m_serializer.signal_new_buffer.connect( boost::bind( &blockbuster::make_avpacket, this,_1,_2) );
}

void blockbuster::serialize_avpacket(AVPacket *pkt)
{
    //Get index in serializer where start of packet is:
    uint32_t index = m_serializer.feed(pkt->data, pkt->size);
    serialized_buffer_table.push_back(index);
    std::cout << "Appended AVPacket to serializer's datavector. AVPacket can now be free'd.";
}

void blockbuster::make_avpacket(uint8_t* ptr, uint32_t size)
{
    AVPacket new_avpacket;
    av_init_packet(&new_avpacket);
    new_avpacket.size = size;
    new_avpacket.data = av_malloc(size);
    memcpy(new_avpacket.data, ptr, size);

    signal_new_avpacket(&new_avpacket);
}


void blockbuster::set_layers(uint32_t nb)
{
    layers = nb;
    // make sure that layer size correspond with vector of layer sizes
    // MAKE SURE KODO KOMPONENT KNOWS THAT AMOUNT OF LAYERS CHANGES
}

uint32_t blockbuster::get_layers()
{
    return layers;
}

blockbuster::set_generation_size(uint32_t gen_size)
{
    generation_size = gen_size;
    // MAKE SURE KODO KOMPONENT KNOWS ABOUT CHANGE
}

uint32_t blockbuster::get_generation_size()
{
    return generation_size;
}

void blockbuster::set_field_size(uint32_t q)
{
    field_size = q;
    // KODO SHOULD KNOW THIS
}

uint32_t blockbuster::get_field_size()
{
    return field_size;
}

void blockbuster::set_symbol_size(uint32_t size)
{
    symbol_size = size;
    // Make sure kodo knows this
}

uint32_t blockbuster::get_symbol_size()
{
    return symbol_size;
}

void blockbuster::set_layer_size(uint32_t layer_number, uint32_t size)
{
    // Make sure that layer number does not exceed number of layers
    layer_sizes[layer_number] = size;
    // Make sure kodo knows this!!
}

uint32_t blockbuster::get_layer_size(uint32_t layer_number)
{
    return layer_sizes[layer_number];
}

void blockbuster::prepare_for_kodo_encoder(AVPacket* pkt)
{
    if (pkt->flags == AV_PKT_FLAG_KEY)
    {
        if (m_serializer.size())
        {
            std::vector<uint8_t>& serialized_buffer = m_serializer.serialize();
            // Pass this buffer to kodo encoder. ALONG with other info. Will this buffer be deleted when serializer resets or program goes out of scope?
            // Pass to kodo encoder:
            m_serializer.reset(serialized_buffer.size()); // Reset, and allocate a new vector with same size as previous
            serialized_buffer_table.clear();
        }
    }
    uint32_t index = m_serializer.feed(pkt->data, pkt->size); // Pass packet data to serializer. Index is start of packet
    serialized_buffer_table.push_back(index); // Push start of buffer to serialized buffer table.


//    Look in packet, see if KEYFrame.
//    If keyframe and already data, serialize old data and pass to encoder along with information about number of layers, size of layers, field size, generation size, symbol size along with info on how to seperate layers.
//    Reset serializer with size of old data.
//    pass frame to serializer.
//    Keep track of how to do UEP on the vector (Where to split the shit in windows)

}
