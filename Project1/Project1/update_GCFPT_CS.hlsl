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

        //GCFP_texture���� prev id�� �ش��ϴ� �����͸� �����.
        GCFP_texture[changed_data.prev_id] = -1;
        
        const uint cur_count = GCFP_counter_buffer[cur_gc_index];

        uint2 cur_id = uint2(cur_gc_index, cur_count);
        GCFP_texture[cur_id] = 

        //GCFP_texture���� cur_gc_index�� �����͸� �߰��Ѵ�.
            //����鼭 �ٷ� ���Ӽ��� ���߸� GCFPT_ID�� �ٷιٷ� �����������
        //GCFP_counter_buffer���� prev id�� gc_index�� �ش��ϴ� �����͸� 1 ���ҽ�Ų��.
        changed_data.prev_id.gc_index

        
        //GCFP_counter_buffer���� cur_gc_index�� �ش��ϴ� �����͸� 1 ������Ų��.
        //GCFPT_ID_buffer�� ������Ʈ�Ѵ�.
        
        //Problem 
        //GCFP_texture���� �����͸� ����ٴ� ��� ǥ���� ���ΰ�? 
            //int -1
        //GCFP_texutre���� �������� ���Ӽ��� ��� ������ ���ΰ�? 
            //���⼭ ���Ӽ��� ������ä�� �����ϰ� ���߿� ���Ӽ��� �����ִ°� �����?
            
        
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