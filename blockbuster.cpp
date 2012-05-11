#include "blockbuster.hpp"

blockbuster::blockbuster(bool inbound)
{
    if(inbound)
    {
        m_kodo_decoder = new kodo_decoder();
        benjamin_krebs = new postoffice("11000",1);
        benjamin_krebs->startThread();
    }
    else {
        m_kodo_encoder = new kodo_encoder();
        benjamin_krebs = new postoffice("11000", "255.255.255.255");
    }
    this->inbound = inbound;
    m_serializer = new serializer();
    m_serializer->signal_new_buffer.connect( boost::bind( &blockbuster::make_avpacket, this,_1,_2) );


    symbol_size = 1282;
    layers = 2;
    max_packet_size = 1450;
}

blockbuster::~blockbuster()
{
}

void blockbuster::serialize_avpacket(AVPacket *pkt)
{
    //Get index in serializer where start of packet is:
    uint32_t index = m_serializer->feed(pkt->data, pkt->size);
    serialized_buffer_table.push_back(index);
}

void blockbuster::make_avpacket(uint8_t* ptr, uint32_t size)
{
    AVPacket new_avpacket;
    std::cout << "blockbuster::make_avpacket - creating av_packet\n";
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
//            std::cout << "Keyframe received..\n";
            if (m_serializer->size())
            {
//                std::cout << "waiting for thread to finish..\n";
                transmission_thread.join(); // Wait for previous transmission thread to finish
//                std::cout << "thread finished..\n";

                std::vector<uint8_t>& serialized_buffer = m_serializer->serialize();
                // Pass this buffer to kodo encoder. ALONG with other info. Will this buffer be deleted when serializer resets or program goes out of scope?
                m_kodo_encoder->set_layers(layers);
                uint32_t bufferlength = serialized_buffer.size();

                uint32_t gsize = calculate_generation_size_from_gop_size(bufferlength); //std::ceil( bufferlength/(float)symbol_size );
                uint32_t symb_size = calculate_symbol_size_from_generation_size(gsize);

//                std::cout << "symb_size*gsize and bufferlength: " << symb_size << " " << gsize << " " << symb_size*gsize << " and " << bufferlength << std::endl;

                assert(symb_size*gsize >= bufferlength);
                uint32_t zeropadding_length = symb_size*gsize - bufferlength;
                std::vector<uint8_t> zeropadding(zeropadding_length);
                serialized_buffer.insert(serialized_buffer.end(),zeropadding.begin(),zeropadding.end());
                // Check the length:
                assert(serialized_buffer.size()==symb_size*gsize);
                m_kodo_encoder->set_layers(1);
                m_kodo_encoder->set_generation_size(gsize);
//                m_kodo_encoder->set_layer_size(1,serialized_buffer_table[1]);
                m_kodo_encoder->set_layer_size(1,gsize);
                m_kodo_encoder->set_layer_gamma(1,100);
//                m_kodo_encoder->set_layer_gamma(2,100);
                m_kodo_encoder->set_symbol_size(symb_size);
                m_kodo_encoder->new_generation((char*)&serialized_buffer[0]);
//                std::cout << "Passed generation to encoder. Now what?\n";
                // End of passing to kodo encoder, now what?

                transmission_thread = boost::thread( &blockbuster::transmit_generation, this, symb_size, gsize, 2 );
//                std::cout << "created thread to transmit encoded packets\n";

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

void blockbuster::connect_to_stream()
{
    std::cout << "starting mailbox thread\n";
    mailboxthread = boost::thread( &blockbuster::mailbox_thread, this );
}


void blockbuster::disconnect()
{
    mailbox_active = false;
    mailboxthread.join();
    std::cout << "mailboxthread terminated..\n";
}

void blockbuster::mailbox_thread()
{
    mailbox_active = true;
    stamp *hdr = (stamp*)malloc(sizeof(stamp));
    int decoded_generation = 0;
    serial_data *received_data = (serial_data*) malloc(sizeof(serial_data));
    m_kodo_decoder->status_output = false;
    int data_array_size = 2000;
    char data_array[data_array_size];
    std::vector<uint8_t> decode_return;
    ////////////// NEED WAY TO STOP THIS SHIT!!
    for(;;)
    {
        if (!mailbox_active) break;
//        std::cout << "Waiting for packet...\n";
        received_data->size = benjamin_krebs->receive(data_array, hdr); //timeout 1s
        if(!received_data->size)
        {
            boost::this_thread::sleep(boost::posix_time::milliseconds(1000) );
            continue;
        }
//        std::cout << "received packet w. size " << received_data->size << std::endl;
        received_data->data = data_array;
//        std::cout << "passing to decoder... \n";
        decode_return = m_kodo_decoder->decode(hdr,*received_data);
//        std::cout << "decoder returned\n";
        if(m_kodo_decoder->has_finished_decoding() && m_kodo_decoder->get_current_generation_id() != decoded_generation)
        {
//            std::cout << "Decoder finished layer: " << m_kodo_decoder->has_finished_decoding()*1 << std::endl;
//            m_serializer->deserialize_signal((uint8_t*)received_data->data, received_data->size);
            m_serializer->deserialize_signal(decode_return);
            decoded_generation = m_kodo_decoder->get_current_generation_id();
        }
    }

    // receive from postoffice
    // pass to decoder
    // decode
    // check if finished
    // if finished -> pass to deserializer
    //
}

void blockbuster::transmit_generation(uint32_t symb_size, uint32_t gen_size, float overhead)
{
    assert(overhead >= 1);
    uint32_t totaldata = symb_size*gen_size*overhead;
    for(uint32_t sent_data_size = 0 ; sent_data_size < totaldata ; sent_data_size += symb_size)
    {
//        std::cout << "sending packet...\n";
//        int retval = m_kodo_encoder->send_packet(*benjamin_krebs);
//        std::cout << "getting packet...\n";
        serial_data packet = m_kodo_encoder->get_packet(1);
//        std::cout << "got packet...\n";
        int return_value = benjamin_krebs->send(packet, &(m_kodo_encoder->payload_stamp));
            if (return_value)
                std::cout << std::endl << "ERROR: " << return_value*1 << std::endl;
            assert(!return_value);

//        std::cout << "done sending packet with value " << return_value << "\n";

    }
//    std::cout << "sent ALL the packets!!11!! +overhead from a generation\n";
}

uint32_t blockbuster::calculate_generation_size_from_gop_size(uint32_t gop_size)
{
    return std::ceil(4*max_packet_size-2*sqrt(4*(max_packet_size*max_packet_size)-2*gop_size) );
}
uint32_t blockbuster::calculate_symbol_size_from_generation_size(uint32_t generation_size)
{
    return max_packet_size-generation_size/8+1;
}
