
#include "DeviceLocalData.cpp"

void OnCudaError(cudaError_t err) {
#if 1 // �f�o�b�O�p�i�{�Ԃ͎�菜���j
   printf("[CUDA Error] %s (code: %d)\n", cudaGetErrorString(err), err);
#endif
}
