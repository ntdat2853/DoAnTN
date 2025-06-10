#include "qtthemcongtydialog.h"
#include "ui_qtthemcongtydialog.h"
#include "api.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>
#include <QDebug>
#include <QRegularExpression>

QTThemCongTyDialog::QTThemCongTyDialog(const QString &token, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::QTThemCongTyDialog)
    , networkManager(new QNetworkAccessManager(this))
    , token(token)
{
    ui->setupUi(this);
    setWindowTitle("Thêm công ty");
    connect(ui->pushButtonXacNhan, &QPushButton::clicked, this, &QTThemCongTyDialog::on_pushButtonXacNhan_clicked);
    connect(ui->pushButtonThoat, &QPushButton::clicked, this, &QTThemCongTyDialog::on_pushButtonThoat_clicked);
}

QTThemCongTyDialog::~QTThemCongTyDialog()
{
    delete ui;
}

void QTThemCongTyDialog::on_pushButtonXacNhan_clicked()
{
    QString congTy = ui->lineEditTenCongTy->text().trimmed();
    QString hoVaTen = ui->lineEditTenQuanLy->text().trimmed();
    QString soDienThoai = ui->lineEditSoDienThoai->text().trimmed();
    QString gmail = ui->lineEditGmail->text().trimmed();

    // Kiểm tra dữ liệu đầu vào
    if (congTy.isEmpty() || hoVaTen.isEmpty() || soDienThoai.isEmpty() || gmail.isEmpty()) {
        qDebug() << "Empty input fields detected";
        accept(); // Đóng dialog ngay cả khi dữ liệu trống
        return;
    }

    // Kiểm tra định dạng số điện thoại (ví dụ: 10 chữ số)
    QRegularExpression phoneRegex("^\\d{10}$");
    if (!phoneRegex.match(soDienThoai).hasMatch()) {
        qDebug() << "Invalid phone number format:" << soDienThoai;
        accept(); // Đóng dialog ngay cả khi số điện thoại không hợp lệ
        return;
    }

    // Kiểm tra định dạng email
    QRegularExpression emailRegex("^[a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\\.[a-zA-Z]{2,}$");
    if (!emailRegex.match(gmail).hasMatch()) {
        qDebug() << "Invalid email format:" << gmail;
        accept(); // Đóng dialog ngay cả khi email không hợp lệ
        return;
    }

    QJsonObject json;
    json["TenDangNhap"] = soDienThoai;
    json["MatKhau"] = soDienThoai;
    json["HoVaTen"] = hoVaTen;
    json["SODienThoai"] = soDienThoai;
    json["Gmail"] = gmail;
    json["CongTy"] = congTy;
    json["NgayTao"] = QDate::currentDate().toString("yyyy-MM-dd");

    qDebug() << "Sending JSON to /quanly/create-with-phone/:" << QJsonDocument(json).toJson(QJsonDocument::Indented);

    QNetworkRequest request(QUrl(API+"/quanly/create-with-phone/"));
    request.setRawHeader("Authorization", ("Bearer " + token).toUtf8());
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QNetworkReply *reply = networkManager->post(request, QJsonDocument(json).toJson());
    connect(reply, &QNetworkReply::finished, this, [=]() {
        handleCreateQuanLyReply(reply);
        reply->deleteLater();
    });

    disconnect(ui->pushButtonXacNhan, &QPushButton::clicked, this, &QTThemCongTyDialog::on_pushButtonXacNhan_clicked);
    connect(ui->pushButtonXacNhan, &QPushButton::clicked, this, &QTThemCongTyDialog::on_pushButtonXacNhan_clicked);
}

void QTThemCongTyDialog::on_pushButtonThoat_clicked()
{
    reject();
}

void QTThemCongTyDialog::handleCreateQuanLyReply(QNetworkReply *reply)
{
    if (reply->error() != QNetworkReply::NoError) {
        qDebug() << "Error creating QuanLy:" << reply->errorString();
        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        if (!doc.isNull() && doc.isObject()) {
            QJsonObject obj = doc.object();
            if (obj.contains("detail")) {
                qDebug() << "Server error detail:" << obj["detail"].toString();
            }
        }
    } else {
        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        QJsonObject obj = doc.object();
        qDebug() << "Create QuanLy response:" << QJsonDocument(obj).toJson(QJsonDocument::Indented);
    }

    accept(); // Đóng dialog trong mọi trường hợp
}

