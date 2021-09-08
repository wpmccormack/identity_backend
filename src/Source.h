#ifndef Source_h
#define Source_h

#include <atomic>
#include <filesystem>
#include <string>
#include <memory>

//#include "Framework/Event.h"
//#include "DataFormats/FEDRawDataCollection.h"

namespace edm {
  class Source {
  public:
    explicit Source(int maxEvents, ProductRegistry& reg, std::string const& datadir);

    int maxEvents() const { return maxEvents_; }

    void fill_fromstream(int streamId, ProductRegistry const &reg,char* iRaw);
    // thread safe
    std::unique_ptr<Event> produce(int streamId, ProductRegistry const& reg);

  private:
    int maxEvents_;
    std::atomic<int> numEvents_;
    unsigned int fNFeds;
    EDPutTokenT<FEDRawDataCollection> const rawToken_;
    std::vector<FEDRawDataCollection> raw_;
  };
}  // namespace edm

#endif
