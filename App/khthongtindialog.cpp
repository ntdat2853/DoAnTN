#include "khthongtindialog.h"
#include "ui_khthongtindialog.h"
#include "api.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>
#include <QDebug>
#include <QRegularExpression>

KHThongTinDialog::KHThongTinDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::KHThongTinDialog)
    , networkManager(new QNetworkAccessManager(this))
{
    ui->setupUi(this);
    setWindowTitle("Thông tin khách hàng");
    setAttribute(Qt::WA_QuitOnClose, false); // Ngăn đóng ứng dụng khi dialog đóng
    connect(ui->pushButtonXacNhan, &QPushButton::clicked, this, &KHThongTinDialog::on_pushButtonXacNhan_clicked);
    connect(ui->pushButtonThoat, &QPushButton::clicked, this, &KHThongTinDialog::on_pushButtonThoat_clicked);
    qDebug() << "KHThongTinDialog created";
}

KHThongTinDialog::~KHThongTinDialog()
{
    qDebug() << "KHThongTinDialog destroyed";
    delete ui;
    delete networkManager;
}

void KHThongTinDialog::setTokenAndTenDangNhap(const QString &token, const QString &tenDangNhap)
{
    this->token = token;
    this->tenDangNhap = tenDangNhap;

    QNetworkRequest request(QUrl(API+"/khachhang-info/" + tenDangNhap));
    request.setRawHeader("Authorization", ("Bearer " + token).toUtf8());
    QNetworkReply *reply = networkManager->get(request);
    connect(reply, &QNetworkReply::finished, this, [=]() {
        handleKhachHangInfoReply(reply);
        reply->deleteLater();
    });
}

void KHThongTinDialog::on_pushButtonXacNhan_clicked()
{
    qDebug() << "XacNhan clicked";
    QString hoVaTen = ui->lineEditHoVaTen->text().trimmed();
    QString soDienThoai = ui->lineEditSoDienThoai->text().trimmed();
    QString gmail = ui->lineEditGmail->text().trimmed();
    QString rfid = ui->lineEditRFID->text().trimmed();
    QString soTaiKhoan = ui->lineEditSoTaiKhoan->text().trimmed();
    QString nganHang = ui->comboBoxNganHang->currentText();

    if (hoVaTen.isEmpty()) {
        QMessageBox::warning(this, "Lỗi", "Họ và tên không được để trống.");
        return;
    }
    if (!soDienThoai.isEmpty() && !soDienThoai.contains(QRegularExpression("^\\d{10,15}$"))) {
        QMessageBox::warning(this, "Lỗi", "Số điện thoại không hợp lệ (phải là 10-15 số).");
        return;
    }
    if (!gmail.isEmpty() && !gmail.contains(QRegularExpression("^[\\w\\.-]+@[\\w\\.-]+\\.[a-zA-Z]{2,}$"))) {
        QMessageBox::warning(this, "Lỗi", "Gmail không hợp lệ.");
        return;
    }

    QJsonObject json;
    json["ten_dang_nhap"] = tenDangNhap;
    json["HoVaTen"] = hoVaTen;
    json["SoDienThoai"] = soDienThoai.isEmpty() ? QJsonValue(QJsonValue::Null) : soDienThoai;
    json["Gmail"] = gmail.isEmpty() ? QJsonValue(QJsonValue::Null) : gmail;
    json["RFID"] = rfid.isEmpty() ? QJsonValue(QJsonValue::Null) : rfid;
    json["SoTaiKhoan"] = soTaiKhoan.isEmpty() ? QJsonValue(QJsonValue::Null) : soTaiKhoan;
    json["NganHang"] = nganHang.isEmpty() ? QJsonValue(QJsonValue::Null) : nganHang;
    json["CongTy"] = congTy.isEmpty() ? QJsonValue(QJsonValue::Null) : congTy;

    QNetworkRequest request(QUrl(API+"/khachhang/update-info/"));
    request.setRawHeader("Authorization", ("Bearer " + token).toUtf8());
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QNetworkReply *reply = networkManager->post(request, QJsonDocument(json).toJson());
    connect(reply, &QNetworkReply::finished, this, [=]() {
        handleUpdateInfoReply(reply);
        reply->deleteLater();
    });

    disconnect(ui->pushButtonXacNhan, &QPushButton::clicked, this, &KHThongTinDialog::on_pushButtonXacNhan_clicked);
    connect(ui->pushButtonXacNhan, &QPushButton::clicked, this, &KHThongTinDialog::on_pushButtonXacNhan_clicked);
}

void KHThongTinDialog::on_pushButtonThoat_clicked()
{
    qDebug() << "Thoat clicked, closing dialog";
    reject();
}

void KHThongTinDialog::handleKhachHangInfoReply(QNetworkReply *reply)
{
    if (reply->error() != QNetworkReply::NoError) {
        QMessageBox::critical(this, "Lỗi", "Không thể lấy thông tin khách hàng: " + reply->errorString());
        reject();
        return;
    }

    QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
    QJsonObject obj = doc.object();

    if (obj.contains("detail") && obj["detail"].toString() == "KhachHang not found") {
        QMessageBox::warning(this, "Lỗi", "Không tìm thấy thông tin khách hàng.");
        reject();
        return;
    }

    ui->lineEditHoVaTen->setText(obj["HoVaTen"].toString());
    ui->lineEditSoDienThoai->setText(obj["SoDienThoai"].toString());
    ui->lineEditGmail->setText(obj["Gmail"].toString());
    ui->lineEditRFID->setText(obj["RFID"].toString());
    ui->lineEditSoTaiKhoan->setText(obj["SoTaiKhoan"].toString());
    QString nganHang = obj["NganHang"].toString();
    if (!nganHang.isEmpty()) {
        int index = ui->comboBoxNganHang->findText(nganHang);
        if (index != -1) {
            ui->comboBoxNganHang->setCurrentIndex(index);
        }
    }
    congTy = obj["CongTy"].toString();
    qDebug() << "Loaded customer info, CongTy:" << congTy;
}

void KHThongTinDialog::handleUpdateInfoReply(QNetworkReply *reply)
{
    if (reply->error() != QNetworkReply::NoError) {
        QMessageBox::critical(this, "Lỗi", "Không thể cập nhật thông tin: " + reply->errorString());
        return;
    }

    QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
    QJsonObject obj = doc.object();

    if (obj.contains("detail") && obj["detail"].toString().contains("Updated KhachHang info")) {
        QMessageBox::information(this, "Thành công", "Thông tin khách hàng đã được cập nhật.");
        qDebug() << "Update successful, emitting infoUpdated with name:" << ui->lineEditHoVaTen->text();
        emit infoUpdated(ui->lineEditHoVaTen->text());
        accept(); // Đóng dialog
        qDebug() << "Called accept() to close dialog";
    } else {
        QMessageBox::critical(this, "Lỗi", "Cập nhật thông tin thất bại: " + obj["detail"].toString());
    }
}
