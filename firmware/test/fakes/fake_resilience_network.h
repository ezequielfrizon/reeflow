#pragma once

#include "fakes/fake_wifi_adapter.h"
#include "network/network_types.h"

namespace reeflow::test::fakes {

class FakeResilienceNetwork {
 public:
  void startOnline() { wifi_.simulateSuccessfulConnection("192.168.1.50", -48); }
  void failWifi() { wifi_.simulateConnectionDrop(); }
  void recoverWifi() { wifi_.setConnectedStatus("192.168.1.50", -48); }

  FakeWifiAdapter& wifi() { return wifi_; }

 private:
  FakeWifiAdapter wifi_;
};

}  // namespace reeflow::test::fakes
