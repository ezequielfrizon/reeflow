#pragma once

#include <stdint.h>

#include "core/state/system_state.h"

namespace reeflow::network {

enum class WifiConnectionState {
  kDisabled,
  kDisconnected,
  kConnecting,
  kConnected,
  kReconnectPending,
  kFailed,
};

enum class WifiDisconnectReason {
  kUnknown,
  kCredentialMissing,
  kAuthenticationFailed,
  kAccessPointUnavailable,
  kConnectionTimeout,
  kSignalLost,
  kManualDisconnect,
};

enum class WifiConnectResult {
  kStarted,
  kAlreadyConnected,
  kConnected,
  kCredentialMissing,
  kAuthenticationFailed,
  kAccessPointUnavailable,
  kTimeout,
  kAdapterFailure,
};

enum class InternetStatus {
  kUnknown,
  kAvailable,
  kUnavailable,
  kDnsFailure,
  kTimeout,
};

enum class NtpSyncStatus {
  kNotStarted,
  kSyncing,
  kSynced,
  kTimeout,
  kServerUnavailable,
  kFailed,
};

enum class NtpSyncResult {
  kStarted,
  kAlreadySynced,
  kSynced,
  kTimeout,
  kServerUnavailable,
  kNetworkUnavailable,
  kClientFailure,
};

struct WifiCredentials {
  const char* ssid;
  const char* password;
};

struct WifiStatus {
  WifiConnectionState state;
  WifiDisconnectReason disconnectReason;
  bool hasIpAddress;
  char ipAddress[core::state::kIpAddressMaxLength];
  bool hasRssi;
  int32_t rssi;
};

struct InternetProbeResult {
  InternetStatus status;
  uint32_t checkedAtMillis;
};

struct NtpTimestamp {
  bool available;
  uint32_t epochSeconds;
};

struct NtpStatus {
  NtpSyncStatus status;
  NtpTimestamp timestamp;
  uint32_t lastAttemptMillis;
};

struct NetworkHeartbeatStatus {
  uint32_t lastHeartbeatMillis;
};

}  // namespace reeflow::network
