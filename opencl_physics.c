
#include <CL/cl.h>
#include <CL/cl_platform.h>
#include <stdio.h>

#define ASSERT_NOERROR(err) if (err != CL_SUCCESS) { fprintf(stderr, "OpenCL error %d at line %d\n", err, __LINE__); exit(EXIT_FAILURE); }
#define PRINT_ERROR(err) if (err != CL_SUCCESS) { fprintf(stderr, "OpenCL error %d at line %d\n", err, __LINE__); }

cl_kernel clkernel;
cl_program clprogram;
cl_command_queue clqueue;
cl_context clcontext;
cl_device_id cldevice;

cl_mem gpu_tiles;
cl_mem gpu_masses;
cl_mem gpu_out_forces;

// Function to get OpenCL device info
void print_device_info(cl_device_id device) {
    cl_ulong mem_size;
    size_t work_group_size;

    clGetDeviceInfo(device, CL_DEVICE_GLOBAL_MEM_SIZE, sizeof(cl_ulong), &mem_size, NULL);
    printf("Global memory size: %llu\n", mem_size);

    clGetDeviceInfo(device, CL_DEVICE_LOCAL_MEM_SIZE, sizeof(cl_ulong), &mem_size, NULL);
    printf("Local memory size: %llu\n", mem_size);

    clGetDeviceInfo(device, CL_DEVICE_MAX_WORK_GROUP_SIZE, sizeof(size_t), &work_group_size, NULL);
    printf("Max work group size: %zu\n", work_group_size);

    clGetDeviceInfo(device, CL_DEVICE_MAX_MEM_ALLOC_SIZE, sizeof(cl_ulong), &mem_size, NULL);
    printf("Max memory allocation size: %llu\n", mem_size);
}



int PX_setupCL(){
    cl_platform_id platforms[64];
    unsigned int platformcount;
    cl_int e1 = clGetPlatformIDs(64, platforms, &platformcount);
    ASSERT_NOERROR(e1);

    for (int i = 0; i < platformcount; i++) {
        cl_device_id devices[64];
        unsigned int devicecount;
        cl_int deviceresult = clGetDeviceIDs(
                platforms[i], 
                CL_DEVICE_TYPE_GPU, 
                64, 
                devices, 
                &devicecount
                );

        if(deviceresult != CL_SUCCESS)
            continue;

        char name[128];
        int e7 = clGetDeviceInfo(devices[0], CL_DEVICE_NAME, sizeof(char) * 128, name, NULL);
        ASSERT_NOERROR(e7);
        printf("%s\n", name);

        size_t max_mem;
        int e8 = clGetDeviceInfo(devices[0], CL_DEVICE_MAX_MEM_ALLOC_SIZE, sizeof(max_mem), &max_mem, NULL);
        ASSERT_NOERROR(e8);
        printf("Maximum memory allocation: %zu\n", max_mem);


        cldevice = devices[0];
    }

    cl_int e6;
    clcontext = clCreateContext(NULL, 1, &cldevice, NULL, NULL, &e6);
    ASSERT_NOERROR(e6);

    cl_int e2;
    clqueue = clCreateCommandQueueWithProperties(clcontext, cldevice, NULL, &e2);
    ASSERT_NOERROR(e2);

    FILE *fptr;
    fptr = fopen("calculate_force_kernel.cl", "r");
    ASSERT_NOERROR(fptr == NULL);

    fseek(fptr, 0, SEEK_END);
    size_t program_size = ftell(fptr);
    fseek(fptr, 0, SEEK_SET);
    char *program_source = malloc(program_size + 1); 
    fread(program_source, program_size, 1, fptr);
    fclose(fptr);

    const char *const_program_source = program_source;    

    cl_int e3;
    cl_program program = clCreateProgramWithSource(clcontext, 1, &const_program_source, &program_size, 
            &e3);
    ASSERT_NOERROR(e3);
    //free(program_source);

    cl_int e4 = clBuildProgram(program, 1, &cldevice, NULL, NULL, NULL);
    PRINT_ERROR(e4);
    if(e4 != CL_SUCCESS){
        char log[1024];
        size_t logLength;
        cl_int programbuildErrorresult = clGetProgramBuildInfo(program, 
                cldevice, 
                CL_PROGRAM_BUILD_LOG, 
                1024, 
                log, 
                &logLength);

        ASSERT_NOERROR(programbuildErrorresult);

        if(programbuildErrorresult == CL_SUCCESS){
            printf("%s", log);
        }
        return 1;
    }

    cl_int e5;
    clkernel = clCreateKernel(program, "calculate_force", &e5);
    ASSERT_NOERROR(e5);

    print_device_info(cldevice);

    return 0;
}

int PX_allocate_gpu_buffers(int tile_h, int tile_v){
    int tiles = tile_h * tile_v;
    cl_int e1, e2, e3, e4, e5;
    gpu_tiles = clCreateBuffer(clcontext, CL_MEM_READ_ONLY, sizeof(cl_float2) * tiles, NULL, &e1);
    gpu_masses = clCreateBuffer(clcontext, CL_MEM_READ_ONLY, sizeof(float) * tiles, NULL, &e2);
    gpu_out_forces = clCreateBuffer(clcontext, CL_MEM_WRITE_ONLY, sizeof(cl_float2) * tiles, NULL, &e5);

    ASSERT_NOERROR(e1);
    ASSERT_NOERROR(e2);
    ASSERT_NOERROR(e5);

    return 0;
}

int PX_set_gpu_kernel_args(int tile_h, int tile_v){
    cl_int e1, e2, e3, e4, e5;
    e1 = clSetKernelArg(clkernel, 0, sizeof(cl_mem), (void*)&gpu_tiles);
    e2 = clSetKernelArg(clkernel, 1, sizeof(cl_mem), (void*)&gpu_masses);
    e3 = clSetKernelArg(clkernel, 2, sizeof(cl_mem), (void*)&tile_v);
    e4 = clSetKernelArg(clkernel, 3, sizeof(cl_mem), (void*)&tile_h);
    e5 = clSetKernelArg(clkernel, 4, sizeof(cl_mem), (void*)&gpu_out_forces);

    ASSERT_NOERROR(e1);
    ASSERT_NOERROR(e2);
    ASSERT_NOERROR(e3);
    ASSERT_NOERROR(e4);
    ASSERT_NOERROR(e5);

    return 0;
}

void PX_clearCL(){
    clReleaseMemObject(gpu_tiles);
    clReleaseMemObject(gpu_masses);
    clReleaseMemObject(gpu_out_forces);

    //release memory before this
    clReleaseKernel( clkernel );
    clReleaseProgram( clprogram );
    clReleaseCommandQueue( clqueue );
    clReleaseContext( clcontext );
    //    clReleaseDevice( cldevice );
}



int PX_flag = 0;

void PX_calculate_physics(cl_float2 *positions, cl_float *masses, cl_float *output, int tile_h, int tile_v){
    int size = tile_h * tile_v;
    printf("rendering opencl frame\n");
    cl_int e1, e2, e3, e4, e5;
    e1 = clEnqueueWriteBuffer(clqueue, gpu_tiles, CL_TRUE, 0, 
            sizeof(cl_float2) * size, positions, 0, NULL, NULL); 
    e2 = clEnqueueWriteBuffer(clqueue, gpu_masses, CL_TRUE, 0, 
            sizeof(float) * size, masses, 0, NULL, NULL); 
    ASSERT_NOERROR(e1);
    ASSERT_NOERROR(e2);


    e1 = clSetKernelArg(clkernel, 0, sizeof(cl_mem), (void*)&gpu_tiles);
    e2 = clSetKernelArg(clkernel, 1, sizeof(cl_mem), (void*)&gpu_masses);
    e3 = clSetKernelArg(clkernel, 2, sizeof(cl_mem), (void*)&tile_v);
    e4 = clSetKernelArg(clkernel, 3, sizeof(cl_mem), (void*)&tile_h);
    e5 = clSetKernelArg(clkernel, 4, sizeof(cl_mem), (void*)&gpu_out_forces);

    ASSERT_NOERROR(e1);
    ASSERT_NOERROR(e2);
    ASSERT_NOERROR(e3);
    ASSERT_NOERROR(e4);



    size_t globalWorkSize = size;
    size_t localWorkSize = 1;

    e5 = clEnqueueNDRangeKernel(clqueue, clkernel, 1, NULL, &globalWorkSize,
            &localWorkSize, 0 , NULL, NULL);
    PRINT_ERROR(e5);

    cl_int e6 = clEnqueueReadBuffer(clqueue, gpu_out_forces, CL_TRUE, 0, 
            sizeof(cl_float2) * size, output, 0, NULL, NULL);

    //ASSERT_NOERROR(e6);

    cl_int e7 = clFinish(clqueue);

    //ASSERT_NOERROR(e7);

}
