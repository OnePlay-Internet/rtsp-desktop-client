#include "nvcomputer.h"
#include "nvapp.h"
#include "settings/compatfetcher.h"

#include <QUdpSocket>
#include <QHostInfo>
#include <QNetworkInterface>
#include <QNetworkProxy>

#define SER_NAME "hostname"
#define SER_UUID "uuid"
#define SER_MAC "mac"
#define SER_LOCALADDR "localaddress"
#define SER_LOCALPORT "localport"
#define SER_REMOTEADDR "remoteaddress"
#define SER_REMOTEPORT "remoteport"
#define SER_MANUALADDR "manualaddress"
#define SER_MANUALPORT "manualport"
#define SER_IPV6ADDR "ipv6address"
#define SER_IPV6PORT "ipv6port"
#define SER_APPLIST "apps"
#define SER_SRVCERT "srvcert"
#define SER_CUSTOMNAME "customname"


NvComputer::NvComputer(ServerInfor* serverInfor)
{
    this->appVersion = QString(serverInfor->appVersion);
    this->gfeVersion = QString(serverInfor->gfeVersion);
    this->gpuModel   = QString(serverInfor->gpuModel);

    this->isSupportedServerVersion = CompatFetcher::isGfeVersionSupported(this->gfeVersion);

    this->displayModes[0] = new NvDisplayMode(&serverInfor->displayModes[0]);

    this->serverCodecModeSupport = serverInfor->serverCodecModeSupport;
    this->maxLumaPixelsHEVC = serverInfor->maxLumaPixelsHEVC;

    memcpy(this->activeAddress,serverInfor->Ip,strlen(serverInfor->Ip));
}

bool NvComputer::isReachableOverVpn()
{
    if (strlen(activeAddress) == 0) {
        return false;
    }

    QTcpSocket s;

    s.setProxy(QNetworkProxy::NoProxy);
    s.connectToHost(QString(activeAddress), 5000);
    if (s.waitForConnected(3000)) {
        Q_ASSERT(!s.localAddress().isNull());

        for (const QNetworkInterface& nic : QNetworkInterface::allInterfaces()) {
            // Ensure the interface is up
            if ((nic.flags() & QNetworkInterface::IsUp) == 0) {
                continue;
            }

            for (const QNetworkAddressEntry& addr : nic.addressEntries()) {
                if (addr.ip() == s.localAddress()) {
                    qInfo() << "Found matching interface:" << nic.humanReadableName() << nic.hardwareAddress() << nic.flags();

#if QT_VERSION >= QT_VERSION_CHECK(5, 11, 0)
                    qInfo() << "Interface Type:" << nic.type();
                    qInfo() << "Interface MTU:" << nic.maximumTransmissionUnit();

                    if (nic.type() == QNetworkInterface::Virtual ||
                            nic.type() == QNetworkInterface::Ppp) {
                        // Treat PPP and virtual interfaces as likely VPNs
                        return true;
                    }

                    if (nic.maximumTransmissionUnit() != 0 && nic.maximumTransmissionUnit() < 1500) {
                        // Treat MTUs under 1500 as likely VPNs
                        return true;
                    }
#endif

                    if (nic.flags() & QNetworkInterface::IsPointToPoint) {
                        // Treat point-to-point links as likely VPNs.
                        // This check detects OpenVPN on Unix-like OSes.
                        return true;
                    }

                    if (nic.hardwareAddress().startsWith("00:FF", Qt::CaseInsensitive)) {
                        // OpenVPN TAP interfaces have a MAC address starting with 00:FF on Windows
                        return true;
                    }

                    if (nic.humanReadableName().startsWith("ZeroTier")) {
                        // ZeroTier interfaces always start with "ZeroTier"
                        return true;
                    }

                    if (nic.humanReadableName().contains("VPN")) {
                        // This one is just a final heuristic if all else fails
                        return true;
                    }

                    // Didn't meet any of our VPN heuristics
                    return false;
                }
            }
        }

        qWarning() << "No match found for address:" << s.localAddress();
        return false;
    }
    else {
        // If we fail to connect, just pretend that it's not a VPN
        return false;
    }
}

