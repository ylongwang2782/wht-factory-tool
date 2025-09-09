#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QUdpSocket>
#include <QHostAddress>
#include <QTimer>
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QMessageBox>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QProgressBar>
#include <QWidget>
#include <QHBoxLayout>
#include <QLabel>

// Protocol相关头文件
#include "protocol/ProtocolProcessor.h"
#include "protocol/messages/Backend2Master.h"
#include "protocol/messages/Master2Backend.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void OnConnectClicked();
    void OnDisconnectClicked();
    void OnSendClicked();
    void OnClearSendClicked();
    void OnClearLogClicked();
    void OnReadyRead();
    void OnSocketError(QAbstractSocket::SocketError error);
    void OnQueryDevicesClicked();

private:
    void InitializeUI();
    void LogMessage(const QString &message, const QString &type = "INFO");
    void WriteLogToFile(const QString &message);
    QByteArray HexStringToByteArray(const QString &hexString);
    QString ByteArrayToHexString(const QByteArray &data);
    void UpdateConnectionState(bool connected);
    void ProcessProtocolMessage(const std::vector<uint8_t> &data);
    void HandleDeviceListResponse(const WhtsProtocol::Master2Backend::DeviceListResponseMessage &message);
    void UpdateDeviceTable(const std::vector<WhtsProtocol::Master2Backend::DeviceListResponseMessage::DeviceInfo> &devices);
    void SendDeviceListRequest();
    QWidget* CreateBatteryWidget(uint8_t batteryLevel);

private:
    Ui::MainWindow *ui;
    QUdpSocket *m_pUdpSocket;
    QHostAddress m_localAddress;
    quint16 m_localPort;
    QHostAddress m_remoteAddress;
    quint16 m_remotePort;
    bool m_bConnected;
    QFile *m_pLogFile;
    QTextStream *m_pLogStream;
    
    // Protocol处理器
    WhtsProtocol::ProtocolProcessor *m_pProtocolProcessor;
};
#endif // MAINWINDOW_H
