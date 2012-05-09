#include "blockbuster.hpp"

blockbuster::blockbuster(bool inbound)
{
    if(inbound)
    {
        m_kodo_decoder = new kodo_decoder();
        benjamin_krebs = new postoffice("11000");
    }
    else {
        m_kodo_encoder = new kodo_encoder();
        benjamin_krebs = new postoffice("11000", "255.255.255.255");
    }
    this->inbound = inbound;
    m_serializer = new serializer();
    m_serializer->signal_new_buffer.connect( boost::bind( &blockbuster::make_avpacket, this,_1,_2) );

    symbol_size = 1400;
    layers = 2;
}

blockbuster::~blockbuster()
{
}

void blockbuster::serialize_avpacket(AVPacket *pkt)
{
    //Get index in serializer where start of packet is:
    uint32_t index = m_serializer->feed(pkt->data, pkt->size);
    serialized_buffer_table.push_back(index);
    std::cout << "Appended AVPacket to serializer's datavector. AVPacket can now be free'd.";
}
//AVPacket* pkt
void blockbuster::make_avpacket(uint8_t* ptr, uint32_t size)
{
    AVPacket new_avpacket;
    av_init_packet(&new_avpacket);
    new_avpacket.size = size;
    new_avpacket.data = (uint8_t*)av_malloc(size);
    memcpy(new_avpacket.data, ptr, size);

    signal_new_avpacket(&new_avpacket);
}

void blockbuster::set_layers(uint32_t nb)
{
    if(!inbound)
    {
        layers = nb;
        m_kodo_encoder->set_layers(nb);
    }
    // make sure that layer size correspond with vector of layer sizes
    // MAKE SURE KODO KOMPONENT KNOWS THAT AMOUNT OF LAYERS CHANGES
}

uint32_t blockbuster::get_layers()
{
    return layers;
}

void blockbuster::set_generation_size(uint32_t gen_size)
{
    if(!inbound)
    {
        generation_size = gen_size;
        m_kodo_encoder->set_generation_size(gen_size);
    }
    // MAKE SURE KODO KOMPONENT KNOWS ABOUT CHANGE
}

uint32_t blockbuster::get_generation_size()
{
    return generation_size;
}

void blockbuster::set_field_size(uint32_t q)
{
    if (inbound) field_size = q;
    // KODO SHOULD KNOW THIS
}

uint32_t blockbuster::get_field_size()
{
    return field_size;
}

void blockbuster::set_symbol_size(uint32_t size)
{
    if (inbound)
    {
        symbol_size = size;
        m_kodo_encoder->set_symbol_size(size);
    }
    // Make sure kodo knows this
}

uint32_t blockbuster::get_symbol_size()
{
    return symbol_size;
}

void blockbuster::set_layer_size(uint32_t layer_number, uint32_t size)
{
    // Make sure that layer number does not exceed number of layers
    if (inbound)
    {
        layer_sizes[layer_number] = size;
        m_kodo_encoder->set_layer_size(layer_number, size);
    }
    // Make sure kodo knows this!!
}

uint32_t blockbuster::get_layer_size(uint32_t layer_number)
{
    return layer_sizes[layer_number];
}

void blockbuster::set_layer_gamma(uint32_t layer_number, uint32_t percentage)
{
    m_kodo_encoder->set_layer_gamma(layer_number, percentage);
}

void blockbuster::prepare_for_kodo_encoder(AVPacket* pkt)
{
    if (!inbound)
    {
        if (pkt->flags == AV_PKT_FLAG_KEY)
        {
            if (m_serializer->size())
            {
                std::vector<uint8_t>& serialized_buffer = m_serializer->serialize();
                // Pass this buffer to kodo encoder. ALONG with other info. Will this buffer be deleted when serializer resets or program goes out of scope?
                m_kodo_encoder->set_layers(layers);
                uint32_t bufferlength = serialized_buffer.size();
                uint32_t gsize = std::ceil(bufferlength/symbol_size);
                uint32_t zeropadding = symbol_size*gsize - bufferlength;
                std::fill(serialized_buffer.end(),serialized_buffer.end()+zeropadding,0);

                // Check the length:
                assert(serialized_buffer.size()==symbol_size*gsize);
                m_kodo_encoder->set_layer_size(1,serialized_buffer_table[0]);
                m_kodo_encoder->set_layer_size(2,serialized_buffer.size());
                m_kodo_encoder->set_layer_gamma(1,0);
                m_kodo_encoder->set_layer_gamma(2,100);
                m_kodo_encoder->set_symbol_size(symbol_size);

                m_kodo_encoder->new_generation((char*)&serialized_buffer[0]);
                // End of passing to kodo encoder, now what?

                // Pass to kodo encoder:
                m_serializer->reset(serialized_buffer.size()); // Reset, and allocate a new vector with same size as previous
                serialized_buffer_table.clear();
            }
        }
        serialize_avpacket(pkt);
    }
//    Look in packet, see if KEYFrame.
//    If keyframe and already data, serialize old data and pass to encoder along with information about number of layers, size of layers, field size, generation size, symbol size along with info on how to seperate layers.
//    Reset serializer with size of old data.
//    pass frame to serializer.
//    Keep track of how to do UEP on the vector (Where to split the shit in windows)

}
