##CL Utility Library
 ------------------
 
 CL Utility Library is a increadibly simple OpenCL helper library for rapid prototyping.
 Because it is a single header library it is very easy to integrate into any project.
 Since it is designed just to make the OpenCL C API easier, instead of wrapping it entirely,
 it is very simple, and does not prevent you from using more obscure features.
 
    git: git://github.com/skylersaleh/CLUtilityLibrary.git

## Requirements:
 
1. The OpenCL Header Files for the platform must be included before this.
2. The application must link to the OpenCL run time libraries for the program.

 ##Basic Usage:

~~~

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
 
 ~~~

### Licence:
 
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
 
