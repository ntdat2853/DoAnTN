#include "qlthemkhachhangdialog.h"
#include "ui_qlthemkhachhangdialog.h"
#include "api.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>
#include <QRegularExpression>
#include <QDate>
#include <QTimer>

QLThemKhachHangDialog::QLThemKhachHangDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::QLThemKhachHangDialog)
    , networkManager(new QNetworkAccessManager(this))
{
    ui->setupUi(this);
    setWindowTitle("Thêm khách hàng");
    setAttribute(Qt::WA_QuitOnClose, false);
    connect(ui->pushButtonXacNhan, &QPushButton::clicked, this, &QLThemKhachHangDialog::on_pushButtonXacNhan_clicked);
    connect(ui->pushButtonThoat, &QPushButton::clicked, this, &QLThemKhachHangDialog::on_pushButtonThoat_clicked);
    qDebug() << "QLThemKhachHangDialog created";
}

QLThemKhachHangDialog::~QLThemKhachHangDialog()
{
    qDebug() << "QLThemKhachHangDialog destroyed";
    delete ui;
    delete networkManager;
}

void QLThemKhachHangDialog::setTokenAndTenDangNhap(const QString &token, const QString &tenDangNhap)
{
    this->token = token;
    this->tenDangNhap = tenDangNhap;

    QNetworkRequest request(QUrl(API+"/quanly-info/" + tenDangNhap));
    request.setRawHeader("Authorization", ("Bearer " + token).toUtf8());
    QNetworkReply *reply = networkManager->get(request);
    connect(reply, &QNetworkReply::finished, this, [=]() {
        handleQuanLyInfoReply(reply);
        reply->deleteLater();
    });
}

void QLThemKhachHangDialog::on_pushButtonXacNhan_clicked()
{
    qDebug() << "XacNhan clicked";
    QString hoVaTen = ui->lineEditHoVaTen->text().trimmed();
    QString soDienThoai = ui->lineEditSoDienThoai->text().trimmed();
    QString gmail = ui->lineEditGmail->text().trimmed();
    QString rfid = ui->lineEditRFID->text().trimmed();
    QString soTaiKhoan = ui->lineEditSoTaiKhoan->text().trimmed();
    QString nganHang = ui->comboBoxNganHang->currentText();

    // Validate
    if (hoVaTen.isEmpty()) {
        qDebug() << "Validation failed: Họ và tên không được để trống";
        emit khachHangAdded(); // Vẫn làm mới giao diện
        accept();
        return;
    }
    if (soDienThoai.isEmpty() || !soDienThoai.contains(QRegularExpression("^\\d{10,15}$"))) {
        qDebug() << "Validation failed: Số điện thoại không hợp lệ";
        emit khachHangAdded(); // Vẫn làm mới giao diện
        accept();
        return;
    }
    if (!gmail.isEmpty() && !gmail.contains(QRegularExpression("^[\\w\\.-]+@[\\w\\.-]+\\.[a-zA-Z]{2,}$"))) {
        qDebug() << "Validation failed: Gmail không hợp lệ";
        emit khachHangAdded(); // Vẫn làm mới giao diện
        accept();
        return;
    }
    if (rfid.isEmpty()) {
        qDebug() << "Validation failed: RFID không được để trống";
        emit khachHangAdded(); // Vẫn làm mới giao diện
        accept();
        return;
    }

    // Gửi yêu cầu tạo khách hàng
    QJsonObject json;
    json["TenDangNhap"] = soDienThoai;
    json["MatKhau"] = soDienThoai;
    json["HoVaTen"] = hoVaTen;
    json["SoDienThoai"] = soDienThoai;
    json["Gmail"] = gmail.isEmpty() ? QJsonValue(QJsonValue::Null) : gmail;
    json["RFID"] = rfid;
    json["SoTaiKhoan"] = soTaiKhoan.isEmpty() ? QJsonValue(QJsonValue::Null) : soTaiKhoan;
    json["NganHang"] = nganHang.isEmpty() ? QJsonValue(QJsonValue::Null) : nganHang;
    json["CongTy"] = congTy;
    json["NgayTao"] = QDate::currentDate().toString("yyyy-MM-dd");

    QNetworkRequest request(QUrl(API+"/khachhang/"));
    request.setRawHeader("Authorization", ("Bearer " + token).toUtf8());
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QNetworkReply *reply = networkManager->post(request, QJsonDocument(json).toJson());
    connect(reply, &QNetworkReply::finished, this, [=]() {
        handleCreateKhachHangReply(reply);
        reply->deleteLater();
    });

    disconnect(ui->pushButtonXacNhan, &QPushButton::clicked, this, &QLThemKhachHangDialog::on_pushButtonXacNhan_clicked);
    connect(ui->pushButtonXacNhan, &QPushButton::clicked, this, &QLThemKhachHangDialog::on_pushButtonXacNhan_clicked);
}

void QLThemKhachHangDialog::on_pushButtonThoat_clicked()
{
    qDebug() << "Thoat clicked, closing dialog";
    reject();
}

void QLThemKhachHangDialog::handleQuanLyInfoReply(QNetworkReply *reply)
{
    if (reply->error() != QNetworkReply::NoError) {
        qDebug() << "Failed to get QuanLy info: " << reply->errorString();
        reject();
        return;
    }

    QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
    QJsonObject obj = doc.object();

    if (obj.contains("detail") && obj["detail"].toString() == "QuanLy not found") {
        qDebug() << "QuanLy not found";
        reject();
        return;
    }

    congTy = obj["CongTy"].toString();
    qDebug() << "Loaded manager CongTy:" << congTy;
}

void QLThemKhachHangDialog::handleCreateKhachHangReply(QNetworkReply *reply)
{
    if (reply->error() != QNetworkReply::NoError) {
        qDebug() << "Failed to create KhachHang: " << reply->errorString();
    } else {
        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        QJsonObject obj = doc.object();
        if (obj.contains("IDKhachHang")) {
            qDebug() << "Customer created successfully";
        } else {
            qDebug() << "Failed to create KhachHang: " << obj["detail"].toString();
        }
    }

    emit khachHangAdded(); // Làm mới giao diện trong mọi trường hợp
    accept(); // Đóng dialog
    qDebug() << "Dialog closed after create attempt";
}
