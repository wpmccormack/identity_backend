#include <cassert>
#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>

#include "Source.h"

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

namespace edm {
  Source::Source(int maxEvents, ProductRegistry &reg, std::string const &datadir)
      : maxEvents_(maxEvents), numEvents_(0), rawToken_(reg.produces<FEDRawDataCollection>()) {
    std::cout << "---> source " << datadir << std::endl;
    std::ifstream in_raw((datadir + "/raw.bin").c_str(), std::ios::binary);

    unsigned int nfeds;
    in_raw.exceptions(std::ifstream::badbit);
    in_raw.read(reinterpret_cast<char *>(&nfeds), sizeof(unsigned int));
    std::cout << "---> N feds " << nfeds << " -- " << datadir << std::endl;
    while (not in_raw.eof()) {
      in_raw.exceptions(std::ifstream::badbit | std::ifstream::failbit | std::ifstream::eofbit);
      raw_.emplace_back(readRaw(in_raw, nfeds));

      // next event
      in_raw.exceptions(std::ifstream::badbit);
      in_raw.read(reinterpret_cast<char *>(&nfeds), sizeof(unsigned int));
    }
    fNFeds = nfeds;
    if (maxEvents_ < 0) {
      maxEvents_ = raw_.size();
    }
  }

  std::unique_ptr<Event> Source::produce(int streamId, ProductRegistry const &reg) {
    const int old = numEvents_.fetch_add(1);
    const int iev = old + 1;
    if (old >= maxEvents_) {
      return nullptr;
    }
    auto ev = std::make_unique<Event>(streamId, iev, reg);
    const int index = old % raw_.size();

    ev->emplace(rawToken_, raw_[index]);
    return ev;
  }

  void Source::fill_fromstream(int streamId, ProductRegistry const &reg,char* iRaw) {
    FEDRawDataCollection rawCollection;
    unsigned int pId = 0;
    for (unsigned int ifed = 0; ifed < fNFeds; ++ifed) {
      std::stringstream strValue;
      strValue << iRaw[pId]; pId++;
      unsigned int fedId;   strValue >> fedId;
      strValue << iRaw[pId]; pId++;
      unsigned int fedSize; strValue >> fedSize;
      //FEDRawData &rawData = rawCollection.FEDData(fedId);
      //rawData.data()      = &(iRaw[pId]);
      pId+=fedSize;
    }
    raw_.emplace_back(rawCollection);
  }
}  // namespace edm
