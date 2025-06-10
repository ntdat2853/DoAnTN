#ifndef QLGIAMUDIALOG_H
#define QLGIAMUDIALOG_H

#include <QDialog>
#include <QNetworkAccessManager>
#include <QNetworkReply>

namespace Ui {
class QLGiaMuDialog;
}

class QLGiaMuDialog : public QDialog
{
    Q_OBJECT

public:
    explicit QLGiaMuDialog(QWidget *parent = nullptr);
    ~QLGiaMuDialog();
    void setTokenAndTenDangNhap(const QString &token, const QString &tenDangNhap);

signals:
    void giaMuUpdated(); // Signal báo hiệu giá mủ được cập nhật

private slots:
    void on_pushButtonXacNhan_clicked();
    void on_pushButtonThoat_clicked();
    void handleQuanLyInfoReply(QNetworkReply *reply);
    void handleCreateGiaMuReply(QNetworkReply *reply);

private:
    Ui::QLGiaMuDialog *ui;
    QNetworkAccessManager *networkManager;
    QString token;
    QString tenDangNhap;
    QString congTy;
};

#endif // QLGIAMUDIALOG_H
