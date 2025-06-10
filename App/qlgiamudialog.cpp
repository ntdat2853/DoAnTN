#include "qlgiamudialog.h"
#include "ui_qlgiamudialog.h"
#include "api.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>
#include <QDebug>
#include <QDate>

QLGiaMuDialog::QLGiaMuDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::QLGiaMuDialog)
    , networkManager(new QNetworkAccessManager(this))
{
    ui->setupUi(this);
    setWindowTitle("Cập nhật giá mủ");
    setAttribute(Qt::WA_QuitOnClose, false);
    connect(ui->pushButtonXacNhan, &QPushButton::clicked, this, &QLGiaMuDialog::on_pushButtonXacNhan_clicked);
    connect(ui->pushButtonThoat, &QPushButton::clicked, this, &QLGiaMuDialog::on_pushButtonThoat_clicked);
    qDebug() << "QLGiaMuDialog created";
}

QLGiaMuDialog::~QLGiaMuDialog()
{
    qDebug() << "QLGiaMuDialog destroyed";
    delete ui;
    delete networkManager;
}

void QLGiaMuDialog::setTokenAndTenDangNhap(const QString &token, const QString &tenDangNhap)
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

void QLGiaMuDialog::on_pushButtonXacNhan_clicked()
{
    qDebug() << "XacNhan clicked";
    bool ok1, ok2;
    double giaMuNuoc = ui->lineEditGiaMuNuoc->text().toDouble(&ok1);
    double giaMuTap = ui->lineEditGiaMuTap->text().toDouble(&ok2);

    if (!ok1 || !ok2 || giaMuNuoc <= 0 || giaMuTap <= 0) {
        QMessageBox::warning(this, "Lỗi", "Giá mủ nước và mủ tạp phải là số dương.");
        return;
    }
    if (giaMuNuoc == giaMuTap) {
        QMessageBox::warning(this, "Lỗi", "Giá mủ nước và mủ tạp không được trùng nhau.");
        return;
    }

    QJsonObject json;
    json["NgayThietLap"] = QDate::currentDate().toString("yyyy-MM-dd");
    json["CongTy"] = congTy;
    json["GiaMuNuoc"] = giaMuNuoc;
    json["GiaMuTap"] = giaMuTap;

    QNetworkRequest request(QUrl(API+"/giamu/"));
    request.setRawHeader("Authorization", ("Bearer " + token).toUtf8());
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QNetworkReply *reply = networkManager->put(request, QJsonDocument(json).toJson());
    connect(reply, &QNetworkReply::finished, this, [=]() {
        handleCreateGiaMuReply(reply);
        reply->deleteLater();
    });

    disconnect(ui->pushButtonXacNhan, &QPushButton::clicked, this, &QLGiaMuDialog::on_pushButtonXacNhan_clicked);
    connect(ui->pushButtonXacNhan, &QPushButton::clicked, this, &QLGiaMuDialog::on_pushButtonXacNhan_clicked);
}

void QLGiaMuDialog::on_pushButtonThoat_clicked()
{
    qDebug() << "Thoat clicked, closing dialog";
    reject();
}

void QLGiaMuDialog::handleQuanLyInfoReply(QNetworkReply *reply)
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

    congTy = obj["CongTy"].toString();
    qDebug() << "Loaded manager CongTy:" << congTy;
}

void QLGiaMuDialog::handleCreateGiaMuReply(QNetworkReply *reply)
{
    if (reply->error() != QNetworkReply::NoError) {
        QMessageBox::critical(this, "Lỗi", "Không thể cập nhật giá mủ: " + reply->errorString());
        return;
    }

    QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
    QJsonObject obj = doc.object();

    if (obj.contains("IDGiaMu")) {
        QMessageBox::information(this, "Thành công", "Giá mủ đã được cập nhật thành công.");
        qDebug() << "GiaMu created/updated successfully";
        emit giaMuUpdated(); // Phát signal để thông báo cập nhật
        accept();
        qDebug() << "Called accept() to close dialog";
    } else {
        QMessageBox::critical(this, "Lỗi", "Cập nhật giá mủ thất bại: " + obj["detail"].toString());
    }
}
