struct Neighbor_Information
{
  uint    nbr_fp_index;   // neighbor�� fluid particle index
  uint    neighbor_index; // this�� neighbor���� ���° neighbor���� ��Ÿ���� index
  float3  v_xij;          // neighbor to this vector
  float   distance;
  float   distnace2;      // distance square
};