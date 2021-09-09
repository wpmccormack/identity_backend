<!--
# Copyright (c) 2020, NVIDIA CORPORATION. All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#  * Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
#  * Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
#  * Neither the name of NVIDIA CORPORATION nor the names of its
#    contributors may be used to endorse or promote products derived
#    from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
# EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
# PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
# CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
# EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
# PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
# OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
-->

[![License](https://img.shields.io/badge/License-BSD3-lightgrey.svg)](https://opensource.org/licenses/BSD-3-Clause)

# Triton Inference Server Identity Backend

An example Triton backend that demonstrates most of the Triton Backend
API. You can learn more about backends in the [backend
repo](https://github.com/triton-inference-server/backend). Ask
questions or report problems in the main Triton [issues
page](https://github.com/triton-inference-server/server/issues).

Use cmake to build and install in a local directory.

```
$ mkdir build
$ cd build
$ cmake -DCMAKE_INSTALL_PREFIX:PATH=`pwd`/install ..
$ make install
```

The following required Triton repositories will be pulled and used in
the build. By default the "main" branch/tag will be used for each repo
but the listed CMake argument can be used to override.

* triton-inference-server/backend: -DTRITON_BACKEND_REPO_TAG=[tag]
* triton-inference-server/core: -DTRITON_CORE_REPO_TAG=[tag]
* triton-inference-server/common: -DTRITON_COMMON_REPO_TAG=[tag]


# Steps to run Patatrack as a Service (Phil's setup as of Sep. 9, 2021)

0. You would likely prefer to be working on a machine with a GPU.  You should also have CUDA version 11.2 - the setup works in this version.  Patatrack supposedly does not work in 11.4, and I had difficulty getting patatrack to compile with 11.0
   * Tests of this workflow were done with a google cloud VM with 1 TESLA V100 GPU
   * For CUDA, I believe I followed these commands (https://developer.nvidia.com/cuda-11.2.2-download-archive?target_os=Linux&target_arch=x86_64&target_distro=Debian&target_version=10&target_type=deblocal):
   ```bash
   wget https://developer.download.nvidia.com/compute/cuda/11.2.2/local_installers/cuda-repo-debian10-11-2-local_11.2.2-460.32.03-1_amd64.deb
   sudo dpkg -i cuda-repo-debian10-11-2-local_11.2.2-460.32.03-1_amd64.deb
   sudo apt-key add /var/cuda-repo-debian10-11-2-local/7fa2af80.pub
   sudo add-apt-repository contrib
   sudo apt-get update
   sudo apt-get -y install cuda
   ```
   * If you run nvcc --version, you should get something like:
   ```bash
   nvcc: NVIDIA (R) Cuda compiler driver
   Copyright (c) 2005-2021 NVIDIA Corporation
   Built on Sun_Feb_14_21:12:58_PST_2021
   Cuda compilation tools, release 11.2, V11.2.152
   Build cuda_11.2.r11.2/compiler.29618528_0
   ```
1. Clone this git repository: https://github.com/violatingcp/pixeltrack-standalone/tree/21.02_phil
   * Something to the effect of:
   ```bash
   git clone https://github.com/violatingcp/pixeltrack-standalone.git
   cd pixeltrack-standalone
   git checkout 21.02_phil
   ```
3. Change env.sh to
   ```bash
   #! /bin/bash
   if [ -f .original_env ]; then
      source .original_env
      else
	echo "#! /bin/bash"            > .original_env
     	echo "PATH=$PATH"             >> .original_env
      	echo "LD_LIBRARY_PATH=$LD_LIBRARY_PATH"  >> .original_env
   fi

   export LD_LIBRARY_PATH=/workspace/backend/pixel/external/tbb/lib:/workspace/backend/pixel/external/libbacktrace/lib:/usr/local/cuda/lib64:/workspace/backend/pixel/lib/cuda/:$LD_LIBRARY_PATH
   #/workspace/backend/pixel/external/cupla/lib:/workspace/backend/pixel/external/kokkos/install/lib:$LD_LIBRARY_PATH
   export PATH=/usr/local/cuda/bin:$PATH
   ```
4. Do
   * cp external/tbb/lib/libtbb.so.2 lib/cuda/
   * cp external/libbacktrace/lib/libbacktrace.so.0 lib/cuda/
5. Follow the quick recipe on that repo's README
   ```bash
   make -j`nproc` cuda
   source env.sh
   ./cuda
   ```
   * You should see some statement about how long it took to process 1000 events
6. Now, we're going to follow Yongbin's instructions for custom backends found here: https://github.com/yongbinfeng/TritonCBE
   * back out of the patatrack directory probably
7. It's probably most convenient to do: docker pull yongbinfeng/tritonserver:21.02v2
8. Then do:
   ```bash
   mkdir CustomBackends
   cd CustomBackends
   ```
9. A slight deviation from YB's instructions here.  We want Phil's fork and branch of identity_backend (https://github.com/violatingcp/identity_backend/tree/v21.02_phil):
   ```bash
   git clone https://github.com/violatingcp/identity_backend.git
   cd identity_backend/
   git checkout v21.02_phil
   ```
10. Back to YB's instructions:
    ```bash
    nvidia-docker run -it --gpus=1 -p8020:8000 -p8021:8001 -p8022:8002 --rm --shm-size=1g --ulimit memlock=-1 --ulimit stack=67108864 -v/PATH_TO_CustomBackends/:/workspace/backend yongbinfeng/tritonserver:21.02v2
    cd /workspace/backend/identity_backend/
    mkdir build
    cd build
    cmake -DTRITON_ENABLE_GPU=ON -DCMAKE_INSTALL_PREFIX:PATH=`pwd`/install -DTRITON_BACKEND_REPO_TAG=r21.02 -DTRITON_CORE_REPO_TAG=r21.02 -DTRITON_COMMON_REPO_TAG=r21.02 ..
    make install
    exit
    ```
11. More from YB.  I did this in CustomBackends/identity_backend/, but you might want to do it elsewhere.  My commands from now on will reflect my particular paths:
    ```bash
    mkdir Test
    cd Test
    git clone https://github.com/yongbinfeng/TritonCBE.git
    cd TritonCBE/TestIdentity/identity_fp32/
    ```
12. Now a few extra tricks (note that my patatrack directory is ~/pixeltrack-standalone/.  Replace that with yours):
    ```bash
    cp ~/CustomBackends/identity_backend/build/libtriton_identity.so ./1/
    cp ~/pixeltrack-standalone/lib/cuda/*so* ./1/
    ```
    * Note that that last command should also get libtbb.so.2 and libbacktrace.so.0
13. You also need to add a file in 1/ called plugins.txt, which contains:
    ```bash
    BeamSpotESProducer pluginBeamSpotProducer.so
    BeamSpotToCUDA pluginBeamSpotProducer.so
    BeamSpotToPOD pluginBeamSpotProducer.so
    CAHitNtupletCUDA pluginPixelTriplets.so
    CountValidator pluginValidation.so
    HistoValidator pluginValidation.so
    SiPixelFedCablingMapGPUWrapperESProducer pluginSiPixelClusterizer.so
    SiPixelGainCalibrationForHLTGPUESProducer pluginSiPixelClusterizer.so
    SiPixelRawToClusterCUDA pluginSiPixelClusterizer.so
    SiPixelDigisSoAFromCUDA pluginSiPixelRawToDigi.so
    PixelCPEFastESProducer pluginSiPixelRecHits.so
    PixelTrackSoAFromCUDA pluginPixelTrackFitting.so
    PixelVertexProducerCUDA pluginPixelVertexFinding.so
    PixelVertexSoAFromCUDA pluginPixelVertexFinding.so
    SiPixelRecHitCUDA pluginSiPixelRecHits.so
    ```
14. You need to do cp -r ~/pixeltrack-standalone/data ./1/
15. Now we will start the Triton Server:
    * Do
    ```bash
    nvidia-docker run -it --gpus=1 -p8020:8000 -p8021:8001 -p 8022:8002 --rm -v/home/wmccorma/CustomBackends/identity_backend/Test/TritonCBE/TestIdentity/:/models yongbinfeng/tritonserver:21.02v2
    ```
    * In that container do:
    ```bash
    export LD_LIBRARY_PATH="/models/identity_fp32/1/:/usr/local/cuda/compat/lib:/usr/local/nvidia/lib:/usr/local/nvidia/lib64"
    export LD_PRELOAD="/models/identity_fp32/1/libFramework.so:/models/identity_fp32/1/libCUDACore.so:/models/identity_fp32/1/libtbb.so.2:/models/identity_fp32/1/libCUDADataFormats.so:/models/identity_fp32/1/libCondFormats.so:/models/identity_fp32/1/pluginBeamSpotProducer.so:/models/identity_fp32/1/pluginSiPixelClusterizer.so"

    tritonserver --model-repository=/models/
    ```
    That last command should start the server
