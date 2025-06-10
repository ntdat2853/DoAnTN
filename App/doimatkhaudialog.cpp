#include "doimatkhaudialog.h"
#include "ui_doimatkhaudialog.h"
#include "api.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>
#include <QDebug>

DoiMatKhauDialog::DoiMatKhauDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::DoiMatKhauDialog)
    , networkManager(new QNetworkAccessManager(this))
{
    ui->setupUi(this);
    setWindowTitle("Đổi mật khẩu");
    // Đặt các lineEdit mật khẩu ở chế độ password
    ui->lineEditMatKhauHienTai->setEchoMode(QLineEdit::Password);
    ui->lineEditMatKhauMoi->setEchoMode(QLineEdit::Password);
    ui->lineEditXacNhanMatKhau->setEchoMode(QLineEdit::Password);
    connect(ui->pushButtonXacNhan, &QPushButton::clicked, this, &DoiMatKhauDialog::on_pushButtonXacNhan_clicked);
    connect(ui->pushButtonThoat, &QPushButton::clicked, this, &DoiMatKhauDialog::on_pushButtonThoat_clicked);
}

DoiMatKhauDialog::~DoiMatKhauDialog()
{
    delete ui;
    delete networkManager;
}

void DoiMatKhauDialog::setTokenAndTenDangNhap(const QString &token, const QString &tenDangNhap)
{
    this->token = token;
    this->tenDangNhap = tenDangNhap;
}

void DoiMatKhauDialog::on_pushButtonXacNhan_clicked()
{
    QString currentPassword = ui->lineEditMatKhauHienTai->text().trimmed();
    QString newPassword = ui->lineEditMatKhauMoi->text().trimmed();
    QString confirmPassword = ui->lineEditXacNhanMatKhau->text().trimmed();

    // Validate
    if (currentPassword.isEmpty() || newPassword.isEmpty() || confirmPassword.isEmpty()) {
        QMessageBox::warning(this, "Lỗi", "Vui lòng nhập đầy đủ mật khẩu hiện tại, mật khẩu mới và xác nhận mật khẩu.");
        return;
    }
    if (newPassword != confirmPassword) {
        QMessageBox::warning(this, "Lỗi", "Mật khẩu mới và xác nhận mật khẩu không khớp.");
        return;
    }
    if (newPassword.length() < 6) {
        QMessageBox::warning(this, "Lỗi", "Mật khẩu mới phải có ít nhất 6 ký tự.");
        return;
    }

    // Tạo JSON để đổi mật khẩu
    QJsonObject json;
    json["current_password"] = currentPassword;
    json["new_password"] = newPassword;

    QNetworkRequest request(QUrl(API+"/taikhoan/" + tenDangNhap + "/change-password"));
    request.setRawHeader("Authorization", ("Bearer " + token).toUtf8());
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QNetworkReply *reply = networkManager->put(request, QJsonDocument(json).toJson());
    connect(reply, &QNetworkReply::finished, this, [=]() {
        handleChangePasswordReply(reply);
        reply->deleteLater();
    });

}

void DoiMatKhauDialog::on_pushButtonThoat_clicked()
{
    reject(); // Đóng dialog mà không làm gì
}

void DoiMatKhauDialog::handleChangePasswordReply(QNetworkReply *reply)
{
    if (reply->error() != QNetworkReply::NoError) {
        //QMessageBox::critical(this, "Lỗi", "Không thể đổi mật khẩu: " + reply->errorString());
        return;
    }

    QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
    QJsonObject obj = doc.object();

    if (obj.contains("detail") && obj["detail"].toString() == "Mật khẩu đã được cập nhật") {
        QMessageBox::information(this, "Thành công", "Mật khẩu đã được đổi thành công.");
        accept(); // Đóng dialog
    } else {
        //QMessageBox::critical(this, "Lỗi", "Đổi mật khẩu thất bại: " + obj["detail"].toString());
    }
}
