#include "mainwindow.h"
#include "./ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_pUdpSocket(nullptr)
    , m_localPort(8080)
    , m_remotePort(8081)
    , m_bConnected(false)
    , m_pLogFile(nullptr)
    , m_pLogStream(nullptr)
{
    ui->setupUi(this);
    InitializeUI();
    
    // 创建日志文件
    QString logFileName = QString("udp_debug_%1.log").arg(QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss"));
    m_pLogFile = new QFile(logFileName, this);
    if (m_pLogFile->open(QIODevice::WriteOnly | QIODevice::Append)) {
        m_pLogStream = new QTextStream(m_pLogFile);
    }
    
    LogMessage("UDP调试工具启动");
}

MainWindow::~MainWindow()
{
    if (m_pUdpSocket) {
        m_pUdpSocket->close();
        delete m_pUdpSocket;
    }
    
    if (m_pLogStream) {
        delete m_pLogStream;
    }
    
    if (m_pLogFile) {
        m_pLogFile->close();
    }
    
    delete ui;
}

void MainWindow::InitializeUI()
{
    // 连接信号和槽
    connect(ui->pushButtonConnect, &QPushButton::clicked, this, &MainWindow::OnConnectClicked);
    connect(ui->pushButtonDisconnect, &QPushButton::clicked, this, &MainWindow::OnDisconnectClicked);
    connect(ui->pushButtonSend, &QPushButton::clicked, this, &MainWindow::OnSendClicked);
    connect(ui->pushButtonClearSend, &QPushButton::clicked, this, &MainWindow::OnClearSendClicked);
    connect(ui->pushButtonClearLog, &QPushButton::clicked, this, &MainWindow::OnClearLogClicked);
    
    // 设置发送框回车键发送
    connect(ui->lineEditSendData, &QLineEdit::returnPressed, this, &MainWindow::OnSendClicked);
    
    // 设置窗口大小
    resize(1000, 700);
}

void MainWindow::OnConnectClicked()
{
    if (m_pUdpSocket) {
        delete m_pUdpSocket;
        m_pUdpSocket = nullptr;
    }
    
    // 获取配置参数
    m_localAddress = QHostAddress(ui->lineEditLocalIP->text());
    m_localPort = ui->lineEditLocalPort->text().toUShort();
    m_remoteAddress = QHostAddress(ui->lineEditRemoteIP->text());
    m_remotePort = ui->lineEditRemotePort->text().toUShort();
    
    // 创建UDP socket
    m_pUdpSocket = new QUdpSocket(this);
    
    // 连接信号
    connect(m_pUdpSocket, &QUdpSocket::readyRead, this, &MainWindow::OnReadyRead);
    connect(m_pUdpSocket, QOverload<QAbstractSocket::SocketError>::of(&QAbstractSocket::errorOccurred),
            this, &MainWindow::OnSocketError);
    
    // 绑定本地地址和端口
    if (m_pUdpSocket->bind(m_localAddress, m_localPort)) {
        m_bConnected = true;
        UpdateConnectionState(true);
        LogMessage(QString("UDP连接成功 - 本地: %1:%2, 目标: %3:%4")
                  .arg(m_localAddress.toString())
                  .arg(m_localPort)
                  .arg(m_remoteAddress.toString())
                  .arg(m_remotePort));
    } else {
        LogMessage(QString("UDP连接失败: %1").arg(m_pUdpSocket->errorString()), "ERROR");
        delete m_pUdpSocket;
        m_pUdpSocket = nullptr;
        m_bConnected = false;
        UpdateConnectionState(false);
    }
}

void MainWindow::OnDisconnectClicked()
{
    if (m_pUdpSocket) {
        m_pUdpSocket->close();
        delete m_pUdpSocket;
        m_pUdpSocket = nullptr;
    }
    
    m_bConnected = false;
    UpdateConnectionState(false);
    LogMessage("UDP连接已断开");
}

void MainWindow::OnSendClicked()
{
    if (!m_bConnected || !m_pUdpSocket) {
        QMessageBox::warning(this, "警告", "请先连接UDP");
        return;
    }
    
    QString hexString = ui->lineEditSendData->text().trimmed();
    if (hexString.isEmpty()) {
        QMessageBox::warning(this, "警告", "请输入要发送的数据");
        return;
    }
    
    // 转换十六进制字符串为字节数组
    QByteArray data = HexStringToByteArray(hexString);
    if (data.isEmpty()) {
        QMessageBox::warning(this, "警告", "无效的十六进制数据");
        return;
    }
    
    // 发送数据
    qint64 bytesWritten = m_pUdpSocket->writeDatagram(data, m_remoteAddress, m_remotePort);
    if (bytesWritten == -1) {
        LogMessage(QString("发送失败: %1").arg(m_pUdpSocket->errorString()), "ERROR");
    } else {
        LogMessage(QString("发送数据 -> %1:%2 [%3] (%4 bytes)")
                  .arg(m_remoteAddress.toString())
                  .arg(m_remotePort)
                  .arg(ByteArrayToHexString(data))
                  .arg(bytesWritten), "SEND");
    }
}

void MainWindow::OnClearSendClicked()
{
    ui->lineEditSendData->clear();
}

void MainWindow::OnClearLogClicked()
{
    ui->textEditLog->clear();
}

void MainWindow::OnReadyRead()
{
    while (m_pUdpSocket && m_pUdpSocket->hasPendingDatagrams()) {
        QByteArray datagram;
        datagram.resize(m_pUdpSocket->pendingDatagramSize());
        
        QHostAddress sender;
        quint16 senderPort;
        
        qint64 bytesRead = m_pUdpSocket->readDatagram(datagram.data(), datagram.size(), &sender, &senderPort);
        
        if (bytesRead > 0) {
            datagram.resize(bytesRead);
            LogMessage(QString("接收数据 <- %1:%2 [%3] (%4 bytes)")
                      .arg(sender.toString())
                      .arg(senderPort)
                      .arg(ByteArrayToHexString(datagram))
                      .arg(bytesRead), "RECV");
        }
    }
}

void MainWindow::OnSocketError(QAbstractSocket::SocketError error)
{
    Q_UNUSED(error)
    if (m_pUdpSocket) {
        LogMessage(QString("UDP错误: %1").arg(m_pUdpSocket->errorString()), "ERROR");
    }
}

void MainWindow::LogMessage(const QString &message, const QString &type)
{
    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz");
    QString logEntry = QString("[%1] [%2] %3").arg(timestamp, type, message);
    
    // 显示在UI中
    ui->textEditLog->append(logEntry);
    
    // 自动滚动到底部
    ui->textEditLog->moveCursor(QTextCursor::End);
    
    // 写入文件（根据内存要求）
    WriteLogToFile(logEntry);
}

void MainWindow::WriteLogToFile(const QString &message)
{
    if (m_pLogStream) {
        *m_pLogStream << message << Qt::endl;
        m_pLogStream->flush();
    }
}

QByteArray MainWindow::HexStringToByteArray(const QString &hexString)
{
    QByteArray result;
    QString cleanHex = hexString.simplified().replace(" ", "").replace(",", "").toUpper();
    
    // 确保长度为偶数
    if (cleanHex.length() % 2 != 0) {
        return QByteArray();
    }
    
    bool ok;
    for (int i = 0; i < cleanHex.length(); i += 2) {
        QString hexByte = cleanHex.mid(i, 2);
        quint8 byte = hexByte.toUInt(&ok, 16);
        if (!ok) {
            return QByteArray();
        }
        result.append(byte);
    }
    
    return result;
}

QString MainWindow::ByteArrayToHexString(const QByteArray &data)
{
    QString result;
    for (int i = 0; i < data.size(); ++i) {
        result += QString("%1").arg(static_cast<quint8>(data[i]), 2, 16, QChar('0')).toUpper();
        if (i < data.size() - 1) {
            result += " ";
        }
    }
    return result;
}

void MainWindow::UpdateConnectionState(bool connected)
{
    ui->pushButtonConnect->setEnabled(!connected);
    ui->pushButtonDisconnect->setEnabled(connected);
    ui->pushButtonSend->setEnabled(connected);
    
    // 更新状态栏
    if (connected) {
        statusBar()->showMessage("已连接");
    } else {
        statusBar()->showMessage("未连接");
    }
}
