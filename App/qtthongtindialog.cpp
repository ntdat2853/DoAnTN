#include "qtthongtindialog.h"
#include "ui_qtthongtindialog.h"
#include "api.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>

QTThongTinDialog::QTThongTinDialog(const QString &token, const QString &tenDangNhap, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::QTThongTinDialog)
    , networkManager(new QNetworkAccessManager(this))
    , token(token)
    , tenDangNhap(tenDangNhap)
{
    ui->setupUi(this);
    setWindowTitle("Thông tin quản trị viên");
    connect(ui->pushButtonXacNhan, &QPushButton::clicked, this, &QTThongTinDialog::on_pushButtonXacNhan_clicked);
    connect(ui->pushButtonThoat, &QPushButton::clicked, this, &QTThongTinDialog::on_pushButtonThoat_clicked);
}

QTThongTinDialog::~QTThongTinDialog()
{
    delete ui;
}

void QTThongTinDialog::setQuanTriInfo(const QString &hoVaTen, const QString &soDienThoai, const QString &gmail)
{
    ui->lineEditHoVaTen->setText(hoVaTen);
    ui->lineEditSoDienThoai->setText(soDienThoai);
    ui->lineEditGmail->setText(gmail);
}

void QTThongTinDialog::on_pushButtonXacNhan_clicked()
{
    QString hoVaTen = ui->lineEditHoVaTen->text().trimmed();
    QString soDienThoai = ui->lineEditSoDienThoai->text().trimmed();
    QString gmail = ui->lineEditGmail->text().trimmed();

    if (hoVaTen.isEmpty() || soDienThoai.isEmpty() || gmail.isEmpty()) {
        QMessageBox::warning(this, "Lỗi", "Vui lòng nhập đầy đủ thông tin");
        return;
    }

    QJsonObject json;
    json["ten_dang_nhap"] = tenDangNhap;
    json["HoVaTen"] = hoVaTen;
    json["SoDienThoai"] = soDienThoai;
    json["Gmail"] = gmail;

    QNetworkRequest request(QUrl(API+"/quantri/update-info/"));
    request.setRawHeader("Authorization", ("Bearer " + token).toUtf8());
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QNetworkReply *reply = networkManager->post(request, QJsonDocument(json).toJson());
    connect(reply, &QNetworkReply::finished, this, [=]() {
        handleUpdateQuanTriReply(reply);
        reply->deleteLater();
    });

    disconnect(ui->pushButtonXacNhan, &QPushButton::clicked, this, &QTThongTinDialog::on_pushButtonXacNhan_clicked);
    connect(ui->pushButtonXacNhan, &QPushButton::clicked, this, &QTThongTinDialog::on_pushButtonXacNhan_clicked);
}

void QTThongTinDialog::on_pushButtonThoat_clicked()
{
    reject();
}

void QTThongTinDialog::handleUpdateQuanTriReply(QNetworkReply *reply)
{
    if (reply->error() != QNetworkReply::NoError) {
        QMessageBox::critical(this, "Lỗi", "Cập nhật thông tin thất bại: " + reply->errorString());
        return;
    }

    QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
    QJsonObject obj = doc.object();

    if (obj.contains("detail") && obj["detail"].toString().contains("Updated QuanTri info")) {
        QMessageBox::information(this, "Thành công", "Thông tin quản trị đã được cập nhật");
        accept();
    } else {
        QMessageBox::critical(this, "Lỗi", "Cập nhật thông tin thất bại: " + obj["detail"].toString());
    }
}
