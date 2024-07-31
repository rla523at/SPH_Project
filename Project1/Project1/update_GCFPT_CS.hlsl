struct Changed_GCFPT_ID_Data
{
    uint2 prev_id;
    uint cur_gc_index;
};

cbuffer Constant_Buffer : register(b0)
{
    uint consume_buffer_count;
}

RWTexture2D<uint> GCFP_texture : register(u0);
RWStructuredBuffer<uint> GCFP_counter_buffer : register(u1);
RWStructuredBuffer<uint2> GCFPT_ID_buffer : register(u2);
ConsumeStructuredBuffer<Changed_GCFPT_ID_Data> changed_GCFPT_ID_buffer : register(u3);

[numthreads(1,1,1)]
void main()
{
    for (int i=0; i < consume_buffer_count; ++i)
    {
        const Changed_GCFPT_ID_Data changed_data =  changed_GCFPT_ID_buffer.Consume();

        const uint fp_index = GCFP_texture[changed_data.prev_id];

        //GCFP_texture에서 prev id에 해당하는 데이터를 지운다.
        GCFP_texture[changed_data.prev_id] = -1;
        
        const uint cur_count = GCFP_counter_buffer[cur_gc_index];

        uint2 cur_id = uint2(cur_gc_index, cur_count);
        GCFP_texture[cur_id] = 

        //GCFP_texture에서 cur_gc_index에 데이터를 추가한다.
            //지우면서 바로 연속성을 맞추면 GCFPT_ID도 바로바로 수정해줘야함
        //GCFP_counter_buffer에서 prev id의 gc_index에 해당하는 데이터를 1 감소시킨다.
        changed_data.prev_id.gc_index

        
        //GCFP_counter_buffer에서 cur_gc_index에 해당하는 데이터를 1 증가시킨다.
        //GCFPT_ID_buffer를 업데이트한다.
        
        //Problem 
        //GCFP_texture에서 데이터를 지운다는 어떻게 표현할 것인가? 
            //int -1
        //GCFP_texutre에서 데이터의 연속성을 어떻게 유지할 것인가? 
            //여기서 연속성을 무시한채로 진행하고 나중에 연속성을 맞춰주는건 어떨가요?
            
        
    }

    uint num_data, size;
    fp_pos_buffer.GetDimensions(num_data, size);

    const uint fp_index = DTID.x;
        
    if (num_data < fp_index) return;

    const float3 pos = fp_pos_buffer[fp_index];

    const GCFPT_ID prev_GCFPT_id = GCFPT_ID_buffer[fp_index];
    const uint cur_gc_index = grid_cell_index(pos);

    if (prev_GCFPT_id.gc_index != cur_gc_index)
    {
        Changed_GCFPT_ID_Data data;
        data.prev_id = prev_GCFPT_id;
        data.cur_gc_index = cur_gc_index;

        updated_GCFPT_ID_Abuffer.Append(data);
    }
}