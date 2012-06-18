#include "blockbuster.hpp"

blockbuster::blockbuster(bool inbound)
{
    if(inbound)
    {
        m_kodo_decoder = new kodo_decoder();
        benjamin_krebs = new postoffice("11000",1);
//        benjamin_krebs->startThread();
    }
    else {
        m_kodo_encoder = new kodo_encoder();
        benjamin_krebs = new postoffice("11000", "255.255.255.255");
    }
    this->inbound = inbound;
    m_serializer = new serializer();
    m_serializer->signal_new_buffer.connect( boost::bind( &blockbuster::make_avpacket, this,_1,_2) );

    max_packet_size = 1450;
    overhead_percentage = 50;
}

blockbuster::~blockbuster()
{
    if(inbound)
    {
        disconnect();
        delete m_kodo_decoder;
    }
    else
    {
        delete m_kodo_encoder;
    }
    delete benjamin_krebs;
    delete m_serializer;
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
//    std::cout << "blockbuster::make_avpacket - creating av_packet\n";
    av_init_packet(&new_avpacket);
    new_avpacket.size = size;
    new_avpacket.data = (uint8_t*)av_malloc(size);
    memcpy(new_avpacket.data, ptr, size);

    signal_new_avpacket(&new_avpacket);
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
//                m_kodo_encoder->set_layers(2);
//                m_kodo_encoder->set_generation_size(gsize);

//                uint32_t first_layer = std::ceil(serialized_buffer_table[1]/(float)symb_size);
//                if (first_layer > gsize) first_layer=gsize;

//                m_kodo_encoder->set_layer_size(1,first_layer);
//                m_kodo_encoder->set_layer_size(2,gsize);
////                m_kodo_encoder->set_layer_size(3,gsize);
//                m_kodo_encoder->set_layer_gamma(1,75);
//                m_kodo_encoder->set_layer_gamma(2,100);
////                m_kodo_encoder->set_layer_gamma(3,100);
//                m_kodo_encoder->set_symbol_size(symb_size);
                make_layers(4,gsize,symb_size);
                m_kodo_encoder->new_generation((char*)&serialized_buffer[0]);
//                std::cout << "Passed generation to encoder. Now what?\n";
                // End of passing to kodo encoder, now what?

                std::cout << "transmitting generation w ID: "<< 1*m_kodo_encoder->payload_stamp.Generation_ID << std::endl;
                transmission_thread = boost::thread( &blockbuster::transmit_generation, this, symb_size, gsize, (1+((float)overhead_percentage)/100) );
//                std::cout << "created thread to transmit encoded packets\n";

                // Pass to kodo encoder:
                m_serializer->reset(serialized_buffer.size()); // Reset, and allocate a new vector with same size as previous
                serialized_buffer_table.clear();
            }
        }
        serialize_avpacket(pkt);
    }
//    pass frame to serializer.
//    Keep track of how to do UEP on the vector (Where to split the shit in windows)
}

void blockbuster::make_layers(uint32_t DUMMY, uint32_t gsize, uint32_t symb_size)
{
	uint32_t nb_layers = number_of_layers;
    m_kodo_encoder->set_generation_size(gsize);
    m_kodo_encoder->set_symbol_size(symb_size);

    uint32_t first_layer;
    first_layer = std::ceil(serialized_buffer_table[1]/(float)symb_size);

    if (first_layer < 32) first_layer = 32; //puha
    else first_layer = calculate_layer_size(first_layer);

    uint32_t first_gamma = gamma[0];
    if (first_layer > gsize)
    {
        first_layer = gsize;
        nb_layers = 1;
        first_gamma = 100;
    }

    m_kodo_encoder->set_layers(nb_layers);

    m_kodo_encoder->set_layer_size(1,first_layer);
    m_kodo_encoder->set_layer_gamma(1,first_gamma);

    uint32_t p_layer_size;
    if (nb_layers > 1)
        p_layer_size = std::ceil( (gsize-first_layer)/(float)(nb_layers-1) );
    for (uint32_t layer = 2; layer < nb_layers ; layer++)
    {
        uint32_t this_layer_size = p_layer_size*(layer-1)+first_layer;
        this_layer_size = calculate_layer_size(this_layer_size);
        if (this_layer_size >= gsize)
        {
            nb_layers = layer;
            break;
        }
        m_kodo_encoder->set_layer_size(layer, this_layer_size );
        m_kodo_encoder->set_layer_gamma(layer, gamma[layer] );
    }
    m_kodo_encoder->set_layers(nb_layers);
    m_kodo_encoder->set_layer_size(nb_layers,gsize);
    m_kodo_encoder->set_layer_gamma(nb_layers,100);
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
    serial_data received_data; // = (serial_data*) malloc(sizeof(serial_data));
    m_kodo_decoder->status_output = false;
    int data_array_size = 2000;
    char data_array[data_array_size];
    std::vector<uint8_t> decode_return;
    ////////////// NEED WAY TO STOP THIS SHIT!!
    while(mailbox_active)
    {
//        std::cout << "Waiting for packet...\n";
        received_data.size = benjamin_krebs->receive(data_array, hdr); //timeout 1s
        if(!received_data.size)
        {
            boost::this_thread::sleep(boost::posix_time::milliseconds(100) );
            continue;
        }
        received_data.data = data_array;
        if (m_kodo_decoder->get_current_generation_id() != hdr->Generation_ID && !m_kodo_decoder->has_finished_decoding())
        {
            std::cout << "failed to decode generation " << m_kodo_decoder->get_current_generation_id()*1 << std::endl;
            std::cout << "Layer 1: " << m_kodo_decoder->is_layer_finish(1)*1 << " Layer 2: " << m_kodo_decoder->is_layer_finish(2)*1 << " Layer 3: " << m_kodo_decoder->is_layer_finish(3)*1 << std::endl;
        }
        if (hdr->Generation_ID != decoded_generation) decode_return = m_kodo_decoder->decode(hdr,received_data);
        if(m_kodo_decoder->has_finished_decoding() && m_kodo_decoder->get_current_generation_id() != decoded_generation)
        {
            std::cout << "Decoder finished layer: " << m_kodo_decoder->has_finished_decoding()*1;
            std::cout << " Gopsize: " << hdr->Generation_Size << " (symbols) Symbolsize: " << hdr->Symbol_Size << std::endl;
            m_serializer->deserialize_signal(decode_return);
            decoded_generation = m_kodo_decoder->get_current_generation_id();
        }
    }
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
        serial_data packet = m_kodo_encoder->get_packet();
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
uint32_t blockbuster::calculate_layer_size(uint32_t layer_size)
{
    uint32_t addition = 0;
    if ( layer_size%8 ) addition = (8-layer_size%8);
    return layer_size + addition;
}
