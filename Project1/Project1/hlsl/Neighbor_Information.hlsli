struct Neighbor_Information
{
  uint    nbr_fp_index;   // neighbor의 fluid particle index
  uint    neighbor_index; // this가 neighbor한테 몇번째 neighbor인지 나타내는 index
  float3  v_xij;          // neighbor to this vector
  float   distance;
  float   distnace2;      // distance square
};