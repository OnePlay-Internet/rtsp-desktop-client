#include "nvpairingmanager.h"
#include "utils.h"

#include <stdexcept>

#include <openssl/bio.h>
#include <openssl/rand.h>
#include <openssl/pem.h>
#include <openssl/x509.h>
#include <openssl/evp.h>

#define REQUEST_TIMEOUT_MS 5000
