#include "qlthongtindialog.h"
#include "ui_qlthongtindialog.h"
#include "api.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>
#include <QDebug>
#include <QRegularExpression>

QLThongTinDialog::QLThongTinDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::QLThongTinDialog)
    , networkManager(new QNetworkAccessManager(this))
{
    ui->setupUi(this);
    setWindowTitle("Thông tin quản lý");
    setAttribute(Qt::WA_QuitOnClose, false);
    connect(ui->pushButtonXacNhan, &QPushButton::clicked, this, &QLThongTinDialog::on_pushButtonXacNhan_clicked);
    connect(ui->pushButtonThoat, &QPushButton::clicked, this, &QLThongTinDialog::on_pushButtonThoat_clicked);
    qDebug() << "QLThongTinDialog created";
}

QLThongTinDialog::~QLThongTinDialog()
{
    qDebug() << "QLThongTinDialog destroyed";
    delete ui;
    delete networkManager;
}

void QLThongTinDialog::setTokenAndTenDangNhap(const QString &token, const QString &tenDangNhap)
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

void QLThongTinDialog::on_pushButtonXacNhan_clicked()
{
    qDebug() << "XacNhan clicked";
    QString hoVaTen = ui->lineEditHoVaTen->text().trimmed();
    QString soDienThoai = ui->lineEditSoDienThoai->text().trimmed();
    QString gmail = ui->lineEditGmail->text().trimmed();
    QString congTy = ui->lineEditTenCongTy->text().trimmed();

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
    if (congTy.isEmpty()) {
        QMessageBox::warning(this, "Lỗi", "Tên công ty không được để trống.");
        return;
    }

    QJsonObject json;
    json["ten_dang_nhap"] = tenDangNhap;
    json["HoVaTen"] = hoVaTen;
    json["SoDienThoai"] = soDienThoai.isEmpty() ? QJsonValue(QJsonValue::Null) : soDienThoai;
    json["Gmail"] = gmail.isEmpty() ? QJsonValue(QJsonValue::Null) : gmail;
    json["CongTy"] = congTy;

    QNetworkRequest request(QUrl(API+"/quanly/update-info/"));
    request.setRawHeader("Authorization", ("Bearer " + token).toUtf8());
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QNetworkReply *reply = networkManager->post(request, QJsonDocument(json).toJson());
    connect(reply, &QNetworkReply::finished, this, [=]() {
        handleUpdateInfoReply(reply);
        reply->deleteLater();
    });

    disconnect(ui->pushButtonXacNhan, &QPushButton::clicked, this, &QLThongTinDialog::on_pushButtonXacNhan_clicked);
    connect(ui->pushButtonXacNhan, &QPushButton::clicked, this, &QLThongTinDialog::on_pushButtonXacNhan_clicked);
}

void QLThongTinDialog::on_pushButtonThoat_clicked()
{
    qDebug() << "Thoat clicked, closing dialog";
    reject();
}

void QLThongTinDialog::handleQuanLyInfoReply(QNetworkReply *reply)
{
    if (reply->error() != QNetworkReply::NoError) {
        QMessageBox::critical(this, "Lỗi", "Không thể lấy thông tin quản lý: " + reply->errorString());
        reject();
        return;
    }

    QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
    QJsonObject obj = doc.object();

    if (obj.contains("detail") && obj["detail"].toString() == "QuanLy not found") {
        QMessageBox::warning(this, "Lỗi", "Không tìm thấy thông tin quản lý.");
        reject();
        return;
    }

    ui->lineEditHoVaTen->setText(obj["HoVaTen"].toString());
    ui->lineEditSoDienThoai->setText(obj["SoDienThoai"].toString());
    ui->lineEditGmail->setText(obj["Gmail"].toString());
    ui->lineEditTenCongTy->setText(obj["CongTy"].toString());
    qDebug() << "Loaded manager info";
}

void QLThongTinDialog::handleUpdateInfoReply(QNetworkReply *reply)
{
    if (reply->error() != QNetworkReply::NoError) {
        QMessageBox::critical(this, "Lỗi", "Không thể cập nhật thông tin: " + reply->errorString());
        return;
    }

    QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
    QJsonObject obj = doc.object();

    if (obj.contains("detail") && obj["detail"].toString().contains("Updated QuanLy info")) {
        QMessageBox::information(this, "Thành công", "Thông tin quản lý đã được cập nhật.");
        qDebug() << "Update successful, emitting infoUpdated with name:" << ui->lineEditHoVaTen->text();
        emit infoUpdated(ui->lineEditHoVaTen->text());
        accept();
        qDebug() << "Called accept() to close dialog";
    } else {
        QMessageBox::critical(this, "Lỗi", "Cập nhật thông tin thất bại: " + obj["detail"].toString());
    }
}
