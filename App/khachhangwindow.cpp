#include "khachhangwindow.h"
#include "ui_khachhangwindow.h"
#include "dangnhapwindow.h"
#include "khthongtindialog.h" // Thêm include
#include "doimatkhaudialog.h"  // Thêm include
#include "api.h"
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QMessageBox>
#include <QTableWidgetItem>
#include <QDebug>
#include <QDate>
#include <QPushButton>
#include <QDialog>
#include <QVBoxLayout>
#include <QTableWidget>
#include <QHeaderView>
#include <QFont>

KhachHangWindow::KhachHangWindow(const QString &token, const QString &tenDangNhap, QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::KhachHangWindow)
    , networkManager(new QNetworkAccessManager(this))
    , token(token)
    , tenDangNhap(tenDangNhap)
    , dangNhapWindow(nullptr)
    , isLogoutProcessed(false) // Khởi tạo trạng thái đăng xuất
    , isThongTinProcessed(false)
    , isDMKProcessed(false)
{
    ui->setupUi(this);
    setWindowTitle("Khách hàng");
    setAttribute(Qt::WA_QuitOnClose, false);

    // Kết nối tín hiệu đăng xuất
    connect(ui->pushButtonDangXuat, &QPushButton::clicked, this, &KhachHangWindow::on_pushButtonDangXuat_clicked);
    connect(ui->pushButtonDoiMatKhau, &QPushButton::clicked, this, &KhachHangWindow::on_pushButtonDoiMatKhau_clicked);
    connect(ui->pushButtonThongTinKhachHang, &QPushButton::clicked, this, &KhachHangWindow::on_pushButtonThongTinKhachHang_clicked);

    // Lấy thông tin khách hàng để lấy IDKhachHang trực tiếp
    QNetworkRequest khachHangRequest(QUrl(API+"/khachhang-info/" + tenDangNhap));
    khachHangRequest.setRawHeader("Authorization", ("Bearer " + token).toUtf8());
    QNetworkReply *khachHangReply = networkManager->get(khachHangRequest);
    connect(khachHangReply, &QNetworkReply::finished, this, [=]() {
        handleKhachHangInfoReply(khachHangReply);
        khachHangReply->deleteLater();
    });
}

KhachHangWindow::~KhachHangWindow()
{
    delete ui;
    delete dangNhapWindow;
}

void KhachHangWindow::on_pushButtonDangXuat_clicked()
{
    isLogoutProcessed = true;
    if (!isLogoutProcessed) {
        qDebug() << "Logout already processed, ignoring";
        return; // Ngăn xử lý nếu đã đăng xuất
    }

    qDebug() << "Processing logout for QuanTriWindow";
    isLogoutProcessed = false;

    // Vô hiệu hóa nút và ngắt kết nối tín hiệu
    ui->pushButtonDangXuat->setEnabled(isLogoutProcessed);
    disconnect(ui->pushButtonDangXuat, &QPushButton::clicked, this, &KhachHangWindow::on_pushButtonDangXuat_clicked);

    dangNhapWindow = new DangNhapWindow(nullptr);
    dangNhapWindow->show();
    this->close();
}

void KhachHangWindow::on_pushButtonDoiMatKhau_clicked()
{
    // isDMKProcessed = true;
    // if (!isDMKProcessed) {
    //     qDebug() << "DMK already processed, ignoring";
    //     return; // Ngăn xử lý nếu đã đăng xuất
    // }

    DoiMatKhauDialog *dialog = new DoiMatKhauDialog(this);
    dialog->setTokenAndTenDangNhap(token, tenDangNhap);
    dialog->exec();
    delete dialog;

    // isDMKProcessed = false;
    // ui->pushButtonDoiMatKhau->setEnabled(isDMKProcessed);
    disconnect(ui->pushButtonDoiMatKhau, &QPushButton::clicked, this, &KhachHangWindow::on_pushButtonDoiMatKhau_clicked);
    connect(ui->pushButtonDoiMatKhau, &QPushButton::clicked, this, &KhachHangWindow::on_pushButtonDoiMatKhau_clicked);
}

void KhachHangWindow::on_pushButtonThongTinKhachHang_clicked()
{
    if (findChild<KHThongTinDialog*>()) {
        qDebug() << "KHThongTinDialog already open, ignoring";
        return;
    }

    KHThongTinDialog *dialog = new KHThongTinDialog(this);
    dialog->setTokenAndTenDangNhap(token, tenDangNhap);
    // Kết nối signal để cập nhật tên
    connect(dialog, &KHThongTinDialog::infoUpdated, this, &KhachHangWindow::updateCustomerName);
    qDebug() << "Opening KHThongTinDialog";
    dialog->exec();
    qDebug() << "KHThongTinDialog closed";
    delete dialog;

    disconnect(ui->pushButtonThongTinKhachHang, &QPushButton::clicked, this, &KhachHangWindow::on_pushButtonThongTinKhachHang_clicked);
    connect(ui->pushButtonThongTinKhachHang, &QPushButton::clicked, this, &KhachHangWindow::on_pushButtonThongTinKhachHang_clicked);
}

void KhachHangWindow::updateCustomerName(const QString &hoVaTen)
{
    // Cập nhật tên trên nút
    ui->pushButtonThongTinKhachHang->setText(hoVaTen.isEmpty() ? "Thông tin khách hàng" : hoVaTen);
}

void KhachHangWindow::on_chiTietButton_clicked(int row)
{
    // Lấy tháng từ dòng được nhấn
    QString thang = ui->tableWidgetLichSu->item(row, 0)->text();
    QString thangFormat = thang.replace("/", "-"); // Chuyển MM/YYYY thành YYYY-MM để so sánh

    // Tạo dialog để hiển thị chi tiết giao dịch
    QDialog *chiTietDialog = new QDialog(this);
    chiTietDialog->setWindowTitle("Chi tiết giao dịch tháng " + thang);
    chiTietDialog->resize(800, 400);

    QVBoxLayout *layout = new QVBoxLayout(chiTietDialog);
    QTableWidget *chiTietTable = new QTableWidget(chiTietDialog);
    chiTietTable->setColumnCount(9);
    chiTietTable->setHorizontalHeaderLabels({
        "Ngày giao dịch",
        "Thời gian giao dịch",
        "Mủ nước",
        "TSC",
        "Giá mủ nước",
        "Mủ tạp",
        "DRC",
        "Giá mủ tạp",
        "Tổng tiền"
    });
    chiTietTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    // Lọc giao dịch trong tháng được chọn
    QJsonArray giaoDichThang;
    for (const QJsonValue &value : giaoDichList) {
        QJsonObject giaoDich = value.toObject();
        QString ngayGiaoDichStr = giaoDich["NgayGiaoDich"].toString();
        QDate ngayGiaoDich = QDate::fromString(ngayGiaoDichStr, "yyyy-MM-dd");
        QString thangGiaoDich = ngayGiaoDich.toString("yyyy-MM");
        if (thangGiaoDich == thangFormat) {
            giaoDichThang.append(giaoDich);
        }
    }

    chiTietTable->setRowCount(giaoDichThang.size());
    for (int i = 0; i < giaoDichThang.size(); ++i) {
        QJsonObject obj = giaoDichThang[i].toObject();
        chiTietTable->setItem(i, 0, new QTableWidgetItem(obj["NgayGiaoDich"].toString()));
        chiTietTable->setItem(i, 1, new QTableWidgetItem(obj["ThoiGianGiaoDich"].toString()));
        chiTietTable->setItem(i, 2, new QTableWidgetItem(QString::number(obj["MuNuoc"].toDouble())));
        chiTietTable->setItem(i, 3, new QTableWidgetItem(QString::number(obj["TSC"].toDouble())));
        chiTietTable->setItem(i, 4, new QTableWidgetItem(QString::number(obj["GiaMuNuoc"].toDouble())));
        chiTietTable->setItem(i, 5, new QTableWidgetItem(QString::number(obj["MuTap"].toDouble())));
        chiTietTable->setItem(i, 6, new QTableWidgetItem(QString::number(obj["DRC"].toDouble())));
        chiTietTable->setItem(i, 7, new QTableWidgetItem(QString::number(obj["GiaMuTap"].toDouble())));
        chiTietTable->setItem(i, 8, new QTableWidgetItem(QString::number(obj["TongTien"].toDouble())));

        // Căn giữa dữ liệu
        for (int col = 0; col < 9; ++col) {
            if (chiTietTable->item(i, col)) {
                chiTietTable->item(i, col)->setTextAlignment(Qt::AlignCenter);
            }
        }
    }

    // In đậm tiêu đề cột
    QFont headerFont = chiTietTable->horizontalHeader()->font();
    headerFont.setBold(true);
    chiTietTable->horizontalHeader()->setFont(headerFont);

    layout->addWidget(chiTietTable);
    chiTietDialog->setLayout(layout);
    chiTietDialog->exec();
}

void KhachHangWindow::handleKhachHangInfoReply(QNetworkReply *reply)
{
    if (reply->error() != QNetworkReply::NoError) {
        QMessageBox::critical(this, "Lỗi", "Không thể lấy thông tin khách hàng: " + reply->errorString());
        return;
    }

    QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
    QJsonObject obj = doc.object();

    if (obj.contains("detail") && obj["detail"].toString() == "KhachHang not found") {
        QMessageBox::warning(this, "Cảnh báo", "Không tìm thấy thông tin khách hàng cho TenDangNhap: " + tenDangNhap + ". Vui lòng kiểm tra dữ liệu trong cơ sở dữ liệu.");
        return;
    }

    idKhachHang = obj["IDKhachHang"].toInt();
    qDebug() << "Found IDKhachHang:" << idKhachHang;

    QString hoVaTen = obj["HoVaTen"].toString();
    ui->pushButtonThongTinKhachHang->setText(hoVaTen.isEmpty() ? "Thông tin khách hàng" : hoVaTen);

    congTy = obj["CongTy"].toString();
    qDebug() << "CongTy:" << congTy;

    // Lấy giá mủ gần nhất của công ty
    QNetworkRequest giaMuRequest(QUrl(API+"/giamu/latest/" + congTy));
    giaMuRequest.setRawHeader("Authorization", ("Bearer " + token).toUtf8());
    QNetworkReply *giaMuReply = networkManager->get(giaMuRequest);
    connect(giaMuReply, &QNetworkReply::finished, this, [=]() {
        handleGiaMuReply(giaMuReply);
        giaMuReply->deleteLater();
    });

    // Lấy danh sách giao dịch trong tháng của khách hàng
    QNetworkRequest giaoDichRequest(QUrl(API+"/giaodich/khachhang/" + QString::number(idKhachHang)));
    giaoDichRequest.setRawHeader("Authorization", ("Bearer " + token).toUtf8());
    QNetworkReply *giaoDichReply = networkManager->get(giaoDichRequest);
    connect(giaoDichReply, &QNetworkReply::finished, this, [=]() {
        handleGiaoDichReply(giaoDichReply);
        giaoDichReply->deleteLater();
    });

    // Lấy danh sách thanh toán của khách hàng
    QNetworkRequest thanhToanRequest(QUrl(API+"/thanhtoan/khachhang/" + QString::number(idKhachHang)));
    thanhToanRequest.setRawHeader("Authorization", ("Bearer " + token).toUtf8());
    QNetworkReply *thanhToanReply = networkManager->get(thanhToanRequest);
    connect(thanhToanReply, &QNetworkReply::finished, this, [=]() {
        handleThanhToanReply(thanhToanReply);
        thanhToanReply->deleteLater();
    });
}

void KhachHangWindow::handleGiaoDichReply(QNetworkReply *reply)
{
    if (reply->error() != QNetworkReply::NoError) {
        QMessageBox::critical(this, "Lỗi", "Không thể lấy danh sách giao dịch: " + reply->errorString());
        return;
    }

    QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
    QJsonArray array = doc.array();
    qDebug() << "GiaoDich array size:" << array.size();

    // Lưu trữ danh sách giao dịch để sử dụng cho nút Chi tiết
    giaoDichList = array;

    // Lấy tháng và năm hiện tại
    QDate currentDate = QDate::currentDate();
    int currentYear = currentDate.year();
    int currentMonth = currentDate.month();

    // Lọc giao dịch trong tháng hiện tại
    QJsonArray giaoDichTrongThang;
    for (const QJsonValue &value : array) {
        QJsonObject giaoDich = value.toObject();
        QString ngayGiaoDichStr = giaoDich["NgayGiaoDich"].toString();
        QDate ngayGiaoDich = QDate::fromString(ngayGiaoDichStr, "yyyy-MM-dd");
        if (ngayGiaoDich.year() == currentYear && ngayGiaoDich.month() == currentMonth) {
            giaoDichTrongThang.append(giaoDich);
        }
    }
    qDebug() << "GiaoDich trong thang array size:" << giaoDichTrongThang.size();

    // Thiết lập bảng giao dịch trong tháng (9 cột)
    ui->tableWidgetGiaoDichThang->setColumnCount(9);
    ui->tableWidgetGiaoDichThang->setHorizontalHeaderLabels({
        "Ngày giao dịch",
        "Thời gian giao dịch",
        "Mủ nước",
        "TSC",
        "Giá mủ nước",
        "Mủ tạp",
        "DRC",
        "Giá mủ tạp",
        "Tổng tiền"
    });
    ui->tableWidgetGiaoDichThang->setRowCount(giaoDichTrongThang.size());
    ui->tableWidgetGiaoDichThang->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    // In đậm tiêu đề cột
    QFont headerFont = ui->tableWidgetGiaoDichThang->horizontalHeader()->font();
    headerFont.setBold(true);
    ui->tableWidgetGiaoDichThang->horizontalHeader()->setFont(headerFont);

    if (giaoDichTrongThang.isEmpty()) {
        QMessageBox::information(this, "Thông báo", "Không có giao dịch nào trong tháng hiện tại.");
    } else {
        for (int i = 0; i < giaoDichTrongThang.size(); ++i) {
            QJsonObject obj = giaoDichTrongThang[i].toObject();
            ui->tableWidgetGiaoDichThang->setItem(i, 0, new QTableWidgetItem(obj["NgayGiaoDich"].toString()));
            ui->tableWidgetGiaoDichThang->setItem(i, 1, new QTableWidgetItem(obj["ThoiGianGiaoDich"].toString()));
            ui->tableWidgetGiaoDichThang->setItem(i, 2, new QTableWidgetItem(QString::number(obj["MuNuoc"].toDouble())));
            ui->tableWidgetGiaoDichThang->setItem(i, 3, new QTableWidgetItem(QString::number(obj["TSC"].toDouble())));
            ui->tableWidgetGiaoDichThang->setItem(i, 4, new QTableWidgetItem(QString::number(obj["GiaMuNuoc"].toDouble())));
            ui->tableWidgetGiaoDichThang->setItem(i, 5, new QTableWidgetItem(QString::number(obj["MuTap"].toDouble())));
            ui->tableWidgetGiaoDichThang->setItem(i, 6, new QTableWidgetItem(QString::number(obj["DRC"].toDouble())));
            ui->tableWidgetGiaoDichThang->setItem(i, 7, new QTableWidgetItem(QString::number(obj["GiaMuTap"].toDouble())));
            ui->tableWidgetGiaoDichThang->setItem(i, 8, new QTableWidgetItem(QString::number(obj["TongTien"].toDouble())));

            // Căn giữa dữ liệu
            for (int col = 0; col < 9; ++col) {
                if (ui->tableWidgetGiaoDichThang->item(i, col)) {
                    ui->tableWidgetGiaoDichThang->item(i, col)->setTextAlignment(Qt::AlignCenter);
                }
            }
        }
    }
}

void KhachHangWindow::handleThanhToanReply(QNetworkReply *reply)
{
    if (reply->error() != QNetworkReply::NoError) {
        QMessageBox::critical(this, "Lỗi", "Không thể lấy lịch sử thanh toán: " + reply->errorString());
        return;
    }

    QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
    QJsonArray array = doc.array();
    qDebug() << "ThanhToan array size:" << array.size();

    // Thiết lập bảng lịch sử thanh toán (5 cột: Tháng, Tổng mủ nước, Tổng mủ tạp, Tổng thanh toán, Chi tiết)
    ui->tableWidgetLichSu->setColumnCount(5);
    ui->tableWidgetLichSu->setHorizontalHeaderLabels({
        "Tháng",
        "Tổng mủ nước",
        "Tổng mủ tạp",
        "Tổng thanh toán",
        "Chi tiết"
    });
    ui->tableWidgetLichSu->setRowCount(array.size());
    ui->tableWidgetLichSu->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    // In đậm tiêu đề cột
    QFont headerFont = ui->tableWidgetLichSu->horizontalHeader()->font();
    headerFont.setBold(true);
    ui->tableWidgetLichSu->horizontalHeader()->setFont(headerFont);

    if (array.isEmpty()) {
        QMessageBox::information(this, "Thông báo", "Không có lịch sử thanh toán.");
    } else {
        for (int i = 0; i < array.size(); ++i) {
            QJsonObject obj = array[i].toObject();
            ui->tableWidgetLichSu->setItem(i, 0, new QTableWidgetItem(obj["Thang"].toString()));
            ui->tableWidgetLichSu->setItem(i, 1, new QTableWidgetItem(QString::number(obj["TongMuNuoc"].toDouble())));
            ui->tableWidgetLichSu->setItem(i, 2, new QTableWidgetItem(QString::number(obj["TongMuTap"].toDouble())));
            ui->tableWidgetLichSu->setItem(i, 3, new QTableWidgetItem(QString::number(obj["TongThanhToan"].toDouble())));

            // Căn giữa dữ liệu
            for (int col = 0; col < 4; ++col) {
                if (ui->tableWidgetLichSu->item(i, col)) {
                    ui->tableWidgetLichSu->item(i, col)->setTextAlignment(Qt::AlignCenter);
                }
            }

            // Tạo nút Chi tiết
            QPushButton *chiTietButton = new QPushButton("Chi tiết");
            chiTietButton->setStyleSheet("border: none; font-style: italic; text-decoration: underline;");
            connect(chiTietButton, &QPushButton::clicked, this, [=]() {
                on_chiTietButton_clicked(i);
            });
            ui->tableWidgetLichSu->setCellWidget(i, 4, chiTietButton);
        }
    }
}

void KhachHangWindow::handleGiaMuReply(QNetworkReply *reply)
{
    if (reply->error() != QNetworkReply::NoError) {
        QMessageBox::critical(this, "Lỗi", "Không thể lấy giá mủ: " + reply->errorString());
        ui->labelGiaMuNuoc->setText("N/A");
        ui->labelGiaMuTap->setText("N/A");
        return;
    }

    QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
    QJsonObject obj = doc.object();

    if (obj.contains("detail") && obj["detail"].toString().contains("No GiaMu records found")) {
        QMessageBox::warning(this, "Cảnh báo", "Không tìm thấy giá mủ cho công ty: " + congTy);
        ui->labelGiaMuNuoc->setText("N/A");
        ui->labelGiaMuTap->setText("N/A");
        return;
    }

    double giaMuNuoc = obj["GiaMuNuoc"].toDouble();
    double giaMuTap = obj["GiaMuTap"].toDouble();

    ui->labelGiaMuNuoc->setText(QString::number(giaMuNuoc));
    ui->labelGiaMuTap->setText(QString::number(giaMuTap));
}
