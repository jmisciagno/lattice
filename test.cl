#define max(a, b) ((a > b) ? a : b)
#define min(a, b) ((a < b) ? a : b)

__kernel void cl_main(__global unsigned char* inputB, __global int* inputB_offset, __global int* inputB_size,
		      __global unsigned char* output, __global int* output_offset, __global int* output_size,
		      __global int* osc1, __global int* osc2
		      ) {
  int i = get_global_id(0);                                           
  int resolution = 480 * 360;                                                  
  int datasize = (resolution * 3) / 2;                                            

    
  if (i < resolution) {
      unsigned char mask1 = ((inputB[(*inputB_offset * datasize) + i] +
		    output[(((*output_offset + 89) % *output_size) * datasize) + i] -
			 inputB[(((*inputB_offset + 1) % *inputB_size) * datasize) + i]) * (*osc1)) / 32;

      unsigned char mask2 = output[(*output_offset * datasize) + i] = (((inputB[(*inputB_offset * datasize) + i] * 1) +
									((output[(((*output_offset + 59) % *output_size) * datasize) + i] - 0) * (*osc2))) / 32) + 0;

      if ((*osc1) == 1) {
	  output[(*output_offset * datasize) + i] = 0;
	}
      else {
	output[(*output_offset * datasize) + i] = mask1 + mask2;
      }
  }

  else {
    output[(*output_offset * datasize) + i] = 128;
  }
}

