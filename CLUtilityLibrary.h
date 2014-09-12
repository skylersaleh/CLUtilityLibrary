/*
 
 CL Utility Library
 ------------------
 
 CL Utility Library is a increadibly simple OpenCL helper library for rapid prototyping.
 Because it is a single header library it is very easy to integrate into any project.
 Since it is designed just to make the OpenCL C API easier, instead of wrapping it entirely,
 it is very simple, and does not prevent you from using more obscure features.
 
 git: git://github.com/skylersaleh/CLUtilityLibrary.git

 Requirements:
 
    1. The OpenCL Header Files for the platform must be included before this.
    2. The application must link to the OpenCL run time libraries for the program.

 Basic Usage:
 #include <OpenCL/cl.h>
 #include "CLUtilityLibrary.h"
 
 int main(){
    cl::init();
    cl::Program prog;
    prog.source=
    "kernel void add(global float* a, global float*b){\n"
    "   int i = get_global_id(0);\n"
    "   a[i]+=b[i];\n"
    "}";
    prog.build();
    cl_command_queue queue = prog.get_default_queue();
    cl_kernel kern = prog.create_kernel("add");
    cl_mem A_b = cl::create_buffer<float>(100);
    cl_mem B_b = cl::create_buffer<float>(100);
    cl::fill_buffer<float>(queue, A_b, 1., 100);
    cl::fill_buffer<float>(queue, B_b, 2., 100);

    cl::set_kernel_arg(kern, 0, A_b);
    cl::set_kernel_arg(kern, 1, B_b);

    size_t global = 100;
    CL_ERR(err= clEnqueueNDRangeKernel(queue, kern, 1, NULL, &global, NULL, 0, NULL, NULL),
    "Failed to equeue kernel");
    clFinish(queue);
    float *A= cl::map_buffer<float>(queue, A_b, 100);
    for(int i=0;i<100;++i)
    std::cout<<A[i]<<std::endl;
 }
 
 Licence:
 
     Copyright (c) 2014 Skyler Saleh
     
     This software is provided 'as-is', without any express or implied
     warranty. In no event will the authors be held liable for any damages
     arising from the use of this software.
     
     Permission is granted to anyone to use this software for any purpose,
     including commercial applications, and to alter it and redistribute it
     freely, subject to the following restrictions:
     
     1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
     2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
     3. This notice may not be removed or altered from any source distribution.
 
 Change Log:
 
 1.0    Initial Release
 
*/

#ifndef OpenCLTest_CLUtil_h
#define OpenCLTest_CLUtil_h
#include <vector>
#include <iostream>
#include <sstream>
#include <memory>

//Just some simple macros to make the code clearer.
#define CL_GET_VECTOR(vect,function) {cl_uint size=0; function; vect.resize(size); function; vect.resize(size);}
#define CL_GET_STRING(string,function) {size_t size=0; function; string.resize(size-1); function;}
#define CL_ERR(OP, msg){cl_int err=0; OP;if(err){std::cout<<"ERROR("<<err<<"): "<<msg<<std::endl;}}
#define PRINT_VAR(var) #var<<": "<<var
namespace cl{
    
    //=======================//
    // Structure Definitions //
    //=======================//
    
    ////////////////
    // Event List //
    ////////////////
    // Simple stack allocated list for event handling. Useful for making event graphs.
    
    // Ex. Enqueuing a sequence of kernels
    
    // cl::EventList evs;
    // clEnqueueNDRangeKernel(...,0,      NULL,evs.push());
    // clEnqueueNDRangeKernel(...,1,evs.last(),evs.push());
    // clEnqueueNDRangeKernel(...,1,evs.last(),evs.push());
    // clWaitForEvents(evs.size, evs.events);
    // evs.clear();
    
    template<int max_size=10>
    struct EventList{
        enum{kMaxEventListSize=max_size};
        cl_event events[kMaxEventListSize];
        int size;
        EventList(){size = 0;}
        void clear(){size=0;}
        cl_event * last(){return events+size-1;}
        cl_event * push(){return events+(size++);}
    };
    
    ////////////
    // Device //
    ////////////
    
    // Represents a found CL Device.
    // You should not construct one of these yourself.
    // This is created for all devices by cl::init();
    struct Device{
        
        cl_device_id device;        //OpenCL Device
        cl_command_queue queue;     //Device Command Queue
        cl_ulong local_size;        //Local Memory Size
        cl_ulong constant_size;     //Constant Memory Size
        cl_uint cl_major;           //Max CL Version Major Supported
        cl_uint cl_minor;           //Max CL Version Minor Supported
        cl_uint compute_units;      //Number of compute units
        std::string name;           //Name of the device
        std::string vendor;         //Vendor of the device
        std::string profile;        //OpenCL Profile Supported
        std::string extensions;     //Extension String
        std::string version;        //OpenCL Version String
        std::string driver_version; //Driver Version String
        
        std::string predefine;      //Source code that could be appended to a program to
                                    //describe device features.
        cl_device_type type;        //Type of Device (CPU, GPU, etc.)
        
        cl_bool available;          //True if device is available
        cl_bool has_compiler;       //True if device has a compiler
        cl_bool little_endian;      //True if device is little endian
        cl_bool has_images;         //True if device supports images.
        
        //Don't call this.
        Device(cl_device_id dev_id);
        
        //Prints the device information to the screen
        void print();
        //Gets a device info scalar value
        template <typename T>
        T getDeviceInfo(cl_device_id dev,cl_device_info info);
        //Gets a string field from the device info.
        std::string getDeviceInfoString(cl_device_id dev,cl_device_info info);
        ~Device();
    };
    
    /////////////
    // Context //
    /////////////
    
    //Represents an opencl context for a specific platform.
    struct Context{
        cl_platform_id platform;
        //Vector of all devices supported in the context.
        std::vector<std::shared_ptr<Device>> devices;
        std::vector<cl_device_id> device_ids;
        std::vector<cl_command_queue> queues;
        cl_context context;
        //This is called by cl::init
        void init(cl_platform_id plat, cl_context_properties * props, cl_command_queue_properties *queue_props);
    };
    
    /////////////
    // Program //
    /////////////
    
    //Used to contain an OpenCL Program.
    struct Program{
        //Use these traits to specify the needed device capabilities to run
        //a program.
        cl_ulong min_local_size;    //Minimum Local Memory Size
        cl_ulong min_constant_size; //Minimum Constant Memory Size
        cl_uint min_cl_major;       //Minimum CL Major Version
        cl_uint min_cl_minor;       //Minimum CL Minor Version
        
        bool only_gpu;              //Only Use GPU Devices
        bool only_cpu;              //Only Use CPU Devices
        
        //The extensions used in the program
        std::vector<std::string> needed_extension;
        
        cl_bool needs_images;       //True if the kernel needs image support.
        
        std::string source;         //The source code for the program to build
        std::string build_options;  //The desired build options for the program
        std::string debug_name;     //A helpful name to refer to the program as for debug.
        
        cl_program program;         //The CL program object
        
        //The devices the program was compiled for
        std::vector<std::shared_ptr<Device>> devices;
        
        Program();
        //Create the kernel object for the kernel named 'kernel' in the program.
        cl_kernel create_kernel(const std::string &kernel);
        //Get the maximum supported Work Group Size for a kernel running on a device.
        size_t get_work_group_size(cl_kernel kern, std::shared_ptr<Device> dev);
        std::vector<std::shared_ptr<Device> > get_kernel_device(cl_kernel kern, size_t min_wgs=1);
        //Get the default command queue for this program.
        cl_command_queue get_default_queue();
        //Get the default device for the program to run on.
        std::shared_ptr<Device> get_default_device();
        //Build the program.
        void build();
        void get_suitable_devices(std::vector<std::shared_ptr<Device>> &devs);
        
        
    };
    
    //==================//
    // OpenCL functions //
    //==================//
    
    ///////////////////////
    // General Functions //
    ///////////////////////
    
    //Initialize the OpenCL Context
    static void init(size_t platform=0,
                     cl_context_properties *props=NULL,
                     cl_command_queue_properties *queue_props=NULL);
    
    //Get the OpenCL Context
    static Context & get_context();
    
    //////////////////////
    // Buffer Functions //
    //////////////////////
    
    //Creates an OpenCL Mem Object to hold size T's
    template<typename T>
    static cl_mem create_buffer(size_t size,
                                cl_mem_flags flags=CL_MEM_READ_WRITE|CL_MEM_ALLOC_HOST_PTR);
    
    //Maps a cl_mem to host accessible memory and returns the casted pointer.
    template<typename T,cl_map_flags flags = CL_MAP_READ|CL_MAP_WRITE, cl_bool blocking=CL_TRUE>
    static T* map_buffer(cl_command_queue queue,
                         cl_mem mem,
                         size_t size,
                         int ev_size=0,
                         cl_event* ev_list=NULL,
                         cl_event* ev=NULL);
    
    //Copies a pattern 'pattern' repeated 'times' times to a cl_mem object.
    template<typename T>
    static void fill_buffer(cl_command_queue queue,
                            cl_mem mem,T pattern,
                            size_t times,
                            size_t offset = 0,
                            cl_uint ev_list_size =0,
                            const cl_event* ev_list=NULL,
                            cl_event* ev =NULL);
    

    //Copies a pattern 'pattern' repeated 'times' times to a cl_mem object.
    template<typename T>
    static void fill_buffer(cl_command_queue queue,
                        cl_mem mem,
                        T pattern,
                        size_t times,
                        size_t offset,
                        cl_event* ev);

    //Copies a pattern 'pattern' repeated 'times' times to a cl_mem object.
    template<typename T>
    static void fill_buffer(cl_command_queue queue,
                        cl_mem mem,
                        T pattern,
                        size_t times,
                        cl_event* ev);
    
    //////////////////////
    // Kernel Functions //
    //////////////////////
    
    //Sets kernel argument number 'arg' to value 't' for kernel 'kern'
    template<typename T>
    static void set_kernel_arg(cl_kernel kern, int arg,const T&t);
    
    /////////////////////
    // Event Functions //
    /////////////////////
    
    //Gets the run time of a cl_event in nanoseconds.
    static double get_event_run_time(cl_event ev);

    //===================//
    // Utility functions //
    //===================//

    //Converts a scalar type to a string.
    template<typename T>
    static std::string to_string(const T& t);
    
    //=====================//
    // Root Implementation //
    //=====================//
    
    template<typename T>
    static std::string to_string(const T& t){
        std::stringstream s;
        s<<t;
        return s.str();
    }
    static Context & get_context(){
        static Context d;
        return d;
    }
    static void init(size_t platform,
                     cl_context_properties *props,
                     cl_command_queue_properties *queue_props){
        
        std::vector<cl_platform_id> ids;
        CL_GET_VECTOR(ids, clGetPlatformIDs(size, ids.data(), &size));
        if(ids.size()<=platform){
            std::cout<<"ERROR: Platform "<<platform<<" is not present\n";
            platform=0;
            return;
        }
        get_context().init(ids[platform],props,queue_props);
        
    }
   
    template<typename T>
    static cl_mem create_buffer(size_t size, cl_mem_flags flags){
        cl_mem b;
        CL_ERR(b=clCreateBuffer(get_context().context, flags, size*sizeof(T), NULL, &err),"Failed to create buffer");
        return b;
    }
    template<typename T>
    static void fill_buffer(cl_command_queue queue,
                            cl_mem mem,T pattern,
                            size_t times,
                            size_t offset,
                            cl_uint ev_list_size,
                            const cl_event* ev_list,
                            cl_event* ev){
        CL_ERR(err=clEnqueueFillBuffer(queue, mem, (void*)&pattern, sizeof(T), offset, sizeof(T)*times, ev_list_size, ev_list, ev),
               "Failed to fill buffer with "+to_string(pattern));
        
    }
    template<typename T>
    static void fill_buffer(cl_command_queue queue,cl_mem mem,T pattern,size_t times, size_t offset, cl_event* ev){
        CL_ERR(err=clEnqueueFillBuffer(queue, mem, (void*)&pattern, sizeof(T), offset, sizeof(T)*times, 0, NULL, ev),
               "Failed to fill buffer with "+to_string(pattern));
        
    }
    template<typename T>
    static void fill_buffer(cl_command_queue queue,cl_mem mem,T pattern, size_t times, cl_event* ev){
        CL_ERR(err=clEnqueueFillBuffer(queue, mem, (void*)&pattern, sizeof(T), 0, sizeof(T)*times, 0, NULL, ev),
               "Failed to fill buffer with "+to_string(pattern));
        
    }
    template<typename T>
    static void set_kernel_arg(cl_kernel kern, int arg,const T&t){
        CL_ERR(err=clSetKernelArg(kern, arg, sizeof(T),&t),"Failed to set arg "+to_string(arg));
    }
    template<typename T,cl_map_flags flags, cl_bool blocking>
    static T* map_buffer(cl_command_queue queue, cl_mem mem,size_t size, int ev_size, cl_event* ev_list,cl_event* ev){
        void * d;
        CL_ERR(d= clEnqueueMapBuffer(queue, mem, blocking, flags, 0, size*sizeof(T), ev_size, ev_list, ev, &err),"Failed to map buffer");
        return (T*)d;
    }
    static double get_event_run_time(cl_event ev){
        cl_ulong start=0,end=0;
        clRetainEvent(ev);
        CL_ERR(err=clGetEventProfilingInfo(ev, CL_PROFILING_COMMAND_END, sizeof(cl_ulong), &end, NULL),
               "Failed to get profiling info");
        CL_ERR(err=clGetEventProfilingInfo(ev, CL_PROFILING_COMMAND_START, sizeof(cl_ulong), &start, NULL),
               "Failed to get profiling info");
        
        std::cout<<"End: "<<double(end)<<" start: "<<double(start)<<std::endl;
        return double(end) - double(start);

    }
    static void error_notify (
                             const char *errinfo,
                             const void *private_info,
                             size_t cb, 
                             void *user_data
                               ){
        std::cout<<errinfo<<std::endl;
    }
    
    
    //Device Implementation
    
    Device::Device(cl_device_id dev_id){
        device=dev_id;
        cl_command_queue_properties props = CL_QUEUE_PROFILING_ENABLE;
        CL_ERR(queue=clCreateCommandQueue(cl::get_context().context, device, props,&err),"Failed to create command queue");
        
        name            = getDeviceInfoString(device, CL_DEVICE_NAME);
        profile         = getDeviceInfoString(device, CL_DEVICE_PROFILE);
        vendor          = getDeviceInfoString(device, CL_DEVICE_VENDOR);
        version         = getDeviceInfoString(device, CL_DEVICE_VERSION);
        driver_version  = getDeviceInfoString(device, CL_DRIVER_VERSION);
        extensions      = getDeviceInfoString(device, CL_DEVICE_EXTENSIONS);
        
        available       = getDeviceInfo<cl_bool>(device, CL_DEVICE_AVAILABLE);
        has_compiler    = getDeviceInfo<cl_bool>(device, CL_DEVICE_COMPILER_AVAILABLE);
        little_endian   = getDeviceInfo<cl_bool>(device, CL_DEVICE_ENDIAN_LITTLE);
        has_images      = getDeviceInfo<cl_bool>(device, CL_DEVICE_IMAGE_SUPPORT);
        type            = getDeviceInfo<cl_device_type>(device, CL_DEVICE_TYPE);
        
        local_size      = getDeviceInfo<cl_ulong>(device, CL_DEVICE_LOCAL_MEM_SIZE);
        constant_size   = getDeviceInfo<cl_ulong>(device, CL_DEVICE_MAX_CONSTANT_BUFFER_SIZE);
        compute_units   = getDeviceInfo<cl_uint>(device, CL_DEVICE_MAX_COMPUTE_UNITS);
        
        sscanf(version.c_str(), "OpenCL %d.%d",&cl_major,&cl_minor);
        
        predefine+="#define DEV_HAS_IMAGES "+to_string(has_images)+"\n";
        predefine+="#define DEV_LITTLE_ENDIAN "+to_string(little_endian)+"\n";
        predefine+="#define DEV_LOCAL_SIZE "+to_string(local_size)+"\n";
        predefine+="#define DEV_CONSTANT_SIZE "+to_string(constant_size)+"\n";
        predefine+="#define DEV_CL_MAJOR "+to_string(cl_major)+"\n";
        predefine+="#define DEV_CL_MINOR "+to_string(cl_minor)+"\n";
        
        print();
        
    }
    
    void Device::print(){
        std::cout<<"Device"<<std::endl;
        std::cout<<"======"<<std::endl;
        std::cout<<"CL_VERSION: "<<cl_major<<"."<<cl_minor<<std::endl;
        std::cout<<PRINT_VAR(name)<<std::endl;
        std::cout<<PRINT_VAR(profile)<<std::endl;
        std::cout<<PRINT_VAR(version)<<std::endl;
        std::cout<<PRINT_VAR(vendor)<<std::endl;
        std::cout<<PRINT_VAR(driver_version)<<std::endl;
        std::cout<<PRINT_VAR(extensions)<<std::endl;
        std::cout<<PRINT_VAR(available)<<std::endl;
        std::cout<<PRINT_VAR(has_images)<<std::endl;
        std::cout<<PRINT_VAR(has_compiler)<<std::endl;
        std::cout<<PRINT_VAR(little_endian)<<std::endl;
        std::cout<<PRINT_VAR(local_size)<<std::endl;
        std::cout<<PRINT_VAR(constant_size)<<std::endl;
        std::cout<<PRINT_VAR(compute_units)<<std::endl;
        
        
        std::cout<<std::endl;
    }
    template <typename T>
    T Device::getDeviceInfo(cl_device_id dev,cl_device_info info){
        T d;
        clGetDeviceInfo(dev, info, sizeof(T), &d, NULL);
        return d;
    }
    std::string Device::getDeviceInfoString(cl_device_id dev,cl_device_info info){
        std::string d;
        size_t size = 0;
        clGetDeviceInfo(dev, info, 0,NULL, &size);
        d.resize(size);
        clGetDeviceInfo(dev, info, d.size(),(void*)d.data(), &size);
        d.resize(size-1);
        return d;
    }
    Device::~Device(){
        clReleaseDevice(device);
        clReleaseCommandQueue(queue);
        std::cout<<"Released device: "<<name<<std::endl;
    }
    
    //Context Implementation
    void Context::init(cl_platform_id plat, cl_context_properties * props, cl_command_queue_properties *queue_props){
        platform = plat;
        
        CL_GET_VECTOR(device_ids, clGetDeviceIDs(platform, CL_DEVICE_TYPE_ALL, size, device_ids.data(), &size));
        CL_ERR(
               context = clCreateContext(props, (uint)device_ids.size(), device_ids.data(), &error_notify, NULL, &err),
               "Failed to create context"
               );
        
        queues.resize(device_ids.size());
        devices.clear();
        for(int i = 0;i<device_ids.size();++i){
            devices.push_back(std::make_shared<Device>(device_ids[i]));
        }
        std::cout<<"Initialized "<<devices.size()<<" CL devices"<<std::endl;
    }
    
    //Program Implementation
    
    Program::Program(){
        min_local_size=min_constant_size=0;
        min_cl_minor=0;
        min_cl_major=1;
        needs_images=false;
        source="";
        only_gpu =false;
        debug_name="program";
        only_cpu = false;
    }
    cl_kernel Program::create_kernel(const std::string &kernel){
        cl_kernel k;
        CL_ERR(k=clCreateKernel(program, kernel.c_str(), &err), "Failed to create Kernel "+kernel+" in "+debug_name);
        return k;
    }
    size_t Program::get_work_group_size(cl_kernel kern, std::shared_ptr<Device> dev){
        size_t sz;
        CL_ERR(err=clGetKernelWorkGroupInfo(kern, dev->device, CL_KERNEL_WORK_GROUP_SIZE, sizeof(size_t), &sz, NULL),"Failed to get work group info");
        return sz;
    }
    std::vector<std::shared_ptr<Device> > Program::get_kernel_device(cl_kernel kern, size_t min_wgs){
        std::vector<std::shared_ptr<Device> > d;
        get_suitable_devices(d);
        return d;
    }
    cl_command_queue Program::get_default_queue(){
        return devices[0]->queue;
    }
    std::shared_ptr<Device> Program::get_default_device(){
        return devices[0];
    }
    
    void Program::build(){
        size_t size[] = {source.length()};
        const char* src[] = {source.c_str()};
        
        CL_ERR(program=clCreateProgramWithSource(cl::get_context().context, 1, src,size,&err),"Failed to create program");
        std::vector<std::shared_ptr<Device>> devs;
        get_suitable_devices(devs);
        std::vector<cl_device_id> dev_ids;
        for(int i=0;i<devs.size();++i)dev_ids.push_back(devs[i]->device);
        CL_ERR(err=clBuildProgram(program, (uint)dev_ids.size(), dev_ids.data(), build_options.c_str(), NULL,NULL),"Failed to build program");
        devices.clear();
        for(int i=0;i<devs.size();++i){
            std::string build_log;
            CL_GET_STRING(build_log, clGetProgramBuildInfo(program, devs[i]->device, CL_PROGRAM_BUILD_LOG, size,(void*) build_log.c_str(), &size));
            
            cl_build_status status;
            clGetProgramBuildInfo(program, devs[i]->device, CL_PROGRAM_BUILD_STATUS, sizeof(cl_build_status), &status, NULL);
            
            
            if(status!=CL_BUILD_SUCCESS){
                std::cout <<"Failed to build "<<debug_name<<" for device "<<devs[i]->name<<std::endl;
                std::cout<<"Build Log:"<<std::endl;
                std::cout<<build_log<<std::endl;
                
            }else{
                devices.push_back(devs[i]);
            }
        }
        std::cout<<"Compiled "<<debug_name<<" for "<<devices.size()<< " devices"<<std::endl;
    }
    void Program::get_suitable_devices(std::vector<std::shared_ptr<Device>> &devs){
        std::vector<std::shared_ptr<Device>>& ds = cl::get_context().devices;
        devs.clear();
        for(int i=0;i<ds.size();++i){
            if(only_gpu&&ds[i]->type!=CL_DEVICE_TYPE_GPU)continue;
            if(only_cpu&&ds[i]->type!=CL_DEVICE_TYPE_CPU)continue;
            
            if(min_cl_major>ds[i]->cl_major|| (min_cl_major==ds[i]->cl_major&&min_cl_minor>ds[i]->cl_minor))continue;
            if(min_local_size>ds[i]->local_size)continue;
            if(min_constant_size>ds[i]->constant_size)continue;
            devs.push_back(ds[i]);
        }
    }
    
};





#endif
