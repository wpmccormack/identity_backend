#include "CUDACore/Product.h"
#include "CUDACore/ScopedContext.h"
#include "CUDACore/copyAsync.h"
#include "CUDACore/host_noncached_unique_ptr.h"
#include "CUDADataFormats/BeamSpotCUDA.h"
#include "Framework/ProductRegistry.h"
//#include "Framework/EmptyWaitingTask.h"
//#include "Framework/WaitingTask.h"
//#include "Framework/WaitingTaskHolder.h"
#include "DataFormats/FEDRawDataCollection.h"
#include "plugin-SiPixelClusterizer/SiPixelRawToClusterCUDA.cc"

#include "Framework/ESPluginFactory.h"
#include "Framework/PluginFactory.h"
#include "Framework/EventSetup.h"
#include "DataFormats/BeamSpotPOD.h"
//#include "PluginManager.h"
//#include "PluginManager.cc"
#include "Source.h"
#include "Source.cc"
#include "StreamSchedule.h"
#include "StreamSchedule.cc"
#include <cuda_runtime.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
//#include <filesystem>


/*
namespace {
  FEDRawDataCollection readRaw(std::ifstream &is, unsigned int nfeds) {
    FEDRawDataCollection rawCollection;
    for (unsigned int ifed = 0; ifed < nfeds; ++ifed) {
      unsigned int fedId;
      is.read(reinterpret_cast<char *>(&fedId), sizeof(unsigned int));
      unsigned int fedSize;
      is.read(reinterpret_cast<char *>(&fedSize), sizeof(unsigned int));
      FEDRawData &rawData = rawCollection.FEDData(fedId);
      rawData.resize(fedSize);
      is.read(reinterpret_cast<char *>(rawData.data()), fedSize);
    }
    return rawCollection;
  }

}  // namespace
*/

class BSTest  {
public:
  explicit BSTest(std::string const& datadir) : data_(datadir),
						//bsPutToken_{reg_.produces<cms::cuda::Product<BeamSpotCUDA>>()},
						bsHost{cms::cuda::make_host_noncached_unique<BeamSpotPOD>(cudaHostAllocWriteCombined)} {
  						//fEvent(0,0,reg_),
						//fSiPixelRawToClusterCUDA(reg_) {
						  std::cout << "---BS " << std::endl;
						  fEvent = std::make_unique<edm::Event>(0, 0, reg_);
						}
  void loadBS(int iId);//edm::EventSetup& eventSetup);
  void readDummy();
  void Event();
  void setItAll(unsigned int iId,std::vector<std::string> const& esproducers,std::vector<std::string> runs);  
  void runToCompletion();
  cms::cuda::host::unique_ptr<uint32_t[]> getOutput();

private:
  std::string data_;
  const edm::EDPutTokenT<cms::cuda::Product<BeamSpotCUDA> > bsPutToken_;
  cms::cuda::host::noncached::unique_ptr<BeamSpotPOD> bsHost;
  edm::EventSetup fSetup;
  std::unique_ptr<edm::Event>      fEvent;
  std::vector<edm::StreamSchedule> fStream;
  std::vector<FEDRawDataCollection> raw_;
  edm::ProductRegistry reg_;
  edmplugin::PluginManager fPluginManager;
  //SiPixelRawToClusterCUDA fSiPixelRawToClusterCUDA;
  edm::Source *fSource;
};

// explicit ~BSTest() { }

void BSTest::readDummy(){
  std::string fileRaw="/models/identity_fp32/1/data/raw.bin";
  std::ifstream in_raw(fileRaw.c_str(), std::ios::binary);
  unsigned int nfeds;
  in_raw.exceptions(std::ifstream::badbit);
  in_raw.read(reinterpret_cast<char *>(&nfeds), sizeof(unsigned int)); 
  std::cout << "---> nfeds " << nfeds << std::endl;
  while (not in_raw.eof()) {
    in_raw.exceptions(std::ifstream::badbit | std::ifstream::failbit | std::ifstream::eofbit);
    raw_.emplace_back(readRaw(in_raw, nfeds));
    in_raw.exceptions(std::ifstream::badbit);
    in_raw.read(reinterpret_cast<char *>(&nfeds), sizeof(unsigned int));
  }
}
void BSTest::loadBS(int iId){ //,void* iContainer) { 
  auto bs = std::make_unique<BeamSpotPOD>();
  std::string file="/models/identity_fp32/1/data/beamspot.bin";
  std::ifstream in(file.c_str(), std::ios::binary);
  in.exceptions(std::ifstream::badbit | std::ifstream::failbit | std::ifstream::eofbit);
  in.read(reinterpret_cast<char*>(bs.get()), sizeof(BeamSpotPOD));
  std::cout << "---> loading file Done" << std::endl;
  //bsHost = bs;
  //bsHost = static_cast<cms::cuda::host::noncached::unique_ptr<BeamSpotPOD> >(bs);
  fSetup.put(std::move(bs));
  *bsHost = fSetup.get<BeamSpotPOD>();
  std::cout << "---> loading file Done 2" << std::endl;
  //auto bsHost2 = std::move(bs);
  //bsHost = *(dynamic_cast<cms::cuda::host::noncached::unique_ptr<BeamSpotPOD> >(std::move(bs)));
  cms::cuda::ScopedContextProduce ctx{iId};
  BeamSpotCUDA bsDevice(ctx.stream());
  cms::cuda::copyAsync(bsDevice.ptr(), bsHost, ctx.stream());
  auto lEventPtr = fEvent.get();
  ctx.emplace(*lEventPtr, bsPutToken_, std::move(bsDevice));  
}
void BSTest::Event() {
  auto nextEventTaskHolder = edm::WaitingTaskWithArenaHolder();
  //edm::EventProcessor processor(
  //maxEvents, numberOfStreams, std::move(edmodules), std::move(esmodules), datadir, validation);
  //auto lEventPtr = fEvent.get();
  //fSiPixelRawToClusterCUDA.doProduce(*lEventPtr,fSetup);//produce(fEvent,fSetup);
  //fSiPixelRawToClusterCUDA.doAcquire(*lEventPtr,fSetup,nextEventTaskHolder);
}
void BSTest::setItAll(unsigned int iId,std::vector<std::string> const& esproducers,std::vector<std::string> runs) { 
  std::string datadir = "/models/identity_fp32/1/data";
  std::cout << "---> loading data " << std::endl;
  fSource = new edm::Source(10, reg_, datadir);
  std::cout << "---> loading data done " << std::endl;
  for (auto const& name : esproducers) {
    std::cout << "---> filling producers " << std::endl;
    fPluginManager.load(name);
    std::cout << "---> Done Loading " << std::endl;
    //std::filesystem::path datadir = "/models/identity_fp32/1/data";
    auto esp = edm::ESPluginFactory::create(name, datadir);
    std::cout << "---> filling producers factory" << std::endl;
    esp->produce(fSetup);
  }
  std::cout << "---> producers done adding stream " << std::endl;
  fStream.emplace_back(reg_,fPluginManager,fSource,&fSetup,iId,runs);
}
void BSTest::runToCompletion() {
    // The task that waits for all other work
    auto globalWaitTask = edm::make_empty_waiting_task();
    globalWaitTask->increment_ref_count();
    for (auto& s : fStream) {
      auto pTask = edm::WaitingTaskHolder(globalWaitTask.get());
      s.runToCompletionAsync(pTask);
    }
    globalWaitTask->wait_for_all();
    if (globalWaitTask->exceptionPtr()) {
      std::rethrow_exception(*(globalWaitTask->exceptionPtr()));
    }
}
//void BSTest::fillSource() {
//}

cms::cuda::host::unique_ptr<uint32_t[]> BSTest::getOutput() {
  cms::cuda::host::unique_ptr<uint32_t[]> out;
  auto globalWaitTask = edm::make_empty_waiting_task();
  globalWaitTask->increment_ref_count();
  //for (auto& s : fStream) {
  auto pTask = edm::WaitingTaskHolder(globalWaitTask.get());
  out = fStream[0].processOneEvent(pTask);
  //}
  globalWaitTask->wait_for_all();
  if (globalWaitTask->exceptionPtr()) {
    std::rethrow_exception(*(globalWaitTask->exceptionPtr()));
  }
  return out;
}

/*
oid SiPixelRecHitCUDA::produce(edm::Event& iEvent, const edm::EventSetup& es) {
  PixelCPEFast const& fcpe = es.get<PixelCPEFast>();

  auto const& pclusters = iEvent.get(token_);
  cms::cuda::ScopedContextProduce ctx{pclusters};

  auto const& clusters = ctx.get(pclusters);
  auto const& digis = ctx.get(iEvent, tokenDigi_);
  auto const& bs = ctx.get(iEvent, tBeamSpot);

  auto nHits = clusters.nClusters();
  if (nHits >= TrackingRecHit2DSOAView::maxHits()) {
    std::cout << "Clusters/Hits Overflow " << nHits << " >= " << TrackingRecHit2DSOAView::maxHits() << std::endl;
  }

  ctx.emplace(iEvent,
              tokenHit_,
              gpuAlgo_.makeHitsAsync(digis, clusters, bs, fcpe.getGPUProductAsync(ctx.stream()), ctx.stream()));
}

*/
