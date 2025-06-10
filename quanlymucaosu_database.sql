-- Tạo cơ sở dữ liệu
CREATE DATABASE IF NOT EXISTS DoAnTN;
USE DoAnTN;

-- Tạo bảng TaiKhoan
CREATE TABLE TaiKhoan (
    IDTaiKhoan INT AUTO_INCREMENT PRIMARY KEY,
    TenDangNhap VARCHAR(50) NOT NULL UNIQUE,
    MatKhau VARCHAR(255) NOT NULL,
    NgayTao DATE NOT NULL,
    VaiTro ENUM('KhachHang', 'QuanLy', 'QuanTri') NOT NULL
);

-- Tạo bảng QuanLy
CREATE TABLE QuanLy (
    IDQuanLy INT AUTO_INCREMENT PRIMARY KEY,
    IDTaiKhoan INT NOT NULL,
    HoVaTen VARCHAR(100),
    SoDienThoai VARCHAR(15),
    Gmail VARCHAR(100),
    CongTy VARCHAR(100),
    FOREIGN KEY (IDTaiKhoan) REFERENCES TaiKhoan(IDTaiKhoan),
    UNIQUE (IDTaiKhoan),
    UNIQUE (CongTy)
);

-- Tạo bảng KhachHang
CREATE TABLE KhachHang (
    IDKhachHang INT AUTO_INCREMENT PRIMARY KEY,
    IDTaiKhoan INT NOT NULL,
    HoVaTen VARCHAR(100),
    Gmail VARCHAR(100),
    RFID VARCHAR(50),
    SoDienThoai VARCHAR(15),
    CongTy VARCHAR(100),
    SoTaiKhoan VARCHAR(50),
    NganHang VARCHAR(100),
    FOREIGN KEY (IDTaiKhoan) REFERENCES TaiKhoan(IDTaiKhoan),
    FOREIGN KEY (CongTy) REFERENCES QuanLy(CongTy),
    UNIQUE (IDTaiKhoan)
);

-- Tạo bảng QuanTri
CREATE TABLE QuanTri (
    IDQuanTri INT AUTO_INCREMENT PRIMARY KEY,
    IDTaiKhoan INT NOT NULL,
    HoVaTen VARCHAR(100),
    SoDienThoai VARCHAR(15),
    Gmail VARCHAR(100),
    FOREIGN KEY (IDTaiKhoan) REFERENCES TaiKhoan(IDTaiKhoan),
    UNIQUE (IDTaiKhoan)
);

-- Tạo bảng GiaMu
CREATE TABLE GiaMu (
    IDGiaMu INT AUTO_INCREMENT PRIMARY KEY,
    NgayThietLap DATE NOT NULL,
    CongTy VARCHAR(100) NOT NULL,
    GiaMuNuoc DECIMAL(10, 2) NOT NULL,
    GiaMuTap DECIMAL(10, 2) NOT NULL,
    FOREIGN KEY (CongTy) REFERENCES QuanLy(CongTy)
);

-- Tạo bảng GiaoDich
CREATE TABLE GiaoDich (
    IDGiaoDich INT AUTO_INCREMENT PRIMARY KEY,
    IDKhachHang INT NOT NULL,
    NgayGiaoDich DATE NOT NULL,
    CongTy VARCHAR(100) NOT NULL,
    ThoiGianGiaoDich TIME NOT NULL,
    MuNuoc DECIMAL(10, 2) DEFAULT NULL,
    TSC DECIMAL(10, 2) DEFAULT NULL,
    GiaMuNuoc DECIMAL(10, 2) DEFAULT NULL,
    MuTap DECIMAL(10, 2) DEFAULT NULL,
    DRC DECIMAL(10, 2) DEFAULT NULL,
    GiaMuTap DECIMAL(10, 2) DEFAULT NULL,
    TongTien DECIMAL(10, 2) DEFAULT NULL,
    FOREIGN KEY (IDKhachHang) REFERENCES KhachHang(IDKhachHang),
    FOREIGN KEY (CongTy) REFERENCES QuanLy(CongTy)
);

-- Tạo bảng ThanhToan
CREATE TABLE ThanhToan (
    IDThanhToan INT AUTO_INCREMENT PRIMARY KEY,
    IDKhachHang INT NOT NULL,
    Thang VARCHAR(7) NOT NULL,
    TongMuNuoc DECIMAL(10, 2),
    TongMuTap DECIMAL(10, 2),
    TongThanhToan DECIMAL(10, 2),
    FOREIGN KEY (IDKhachHang) REFERENCES KhachHang(IDKhachHang),
    UNIQUE (IDKhachHang, Thang)
);

-- Tạo bảng TriggerLog
CREATE TABLE TriggerLog (
    LogID INT AUTO_INCREMENT PRIMARY KEY,
    Message VARCHAR(255),
    LogTime DATETIME DEFAULT CURRENT_TIMESTAMP
);

DELIMITER //

-- Trigger để tự động thêm giá mủ cho ngày mới và gán giá mủ cho giao dịch
CREATE TRIGGER before_giaodich_insert
BEFORE INSERT ON GiaoDich
FOR EACH ROW
BEGIN
    DECLARE valid_gia_mu_nuoc DECIMAL(10, 2);
    DECLARE valid_gia_mu_tap DECIMAL(10, 2);
    DECLARE cong_ty VARCHAR(100);
    DECLARE previous_date DATE;
    
    -- Lấy CongTy từ KhachHang
    SELECT CongTy INTO cong_ty
    FROM KhachHang
    WHERE IDKhachHang = NEW.IDKhachHang;
    
    -- Gán CongTy cho bản ghi GiaoDich
    SET NEW.CongTy = cong_ty;
    
    -- Kiểm tra xem ngày giao dịch đã có giá mủ chưa
    IF NOT EXISTS (
        SELECT 1
        FROM GiaMu
        WHERE NgayThietLap = NEW.NgayGiaoDich
        AND CongTy = cong_ty
    ) THEN
        -- Tìm giá mủ của ngày gần nhất trước đó
        SELECT GiaMuNuoc, GiaMuTap, NgayThietLap
        INTO valid_gia_mu_nuoc, valid_gia_mu_tap, previous_date
        FROM GiaMu
        WHERE CongTy = cong_ty
        AND NgayThietLap < NEW.NgayGiaoDich
        ORDER BY NgayThietLap DESC, IDGiaMu DESC
        LIMIT 1;
        
        -- Nếu tìm thấy giá mủ trước đó, thêm bản ghi mới vào GiaMu
        IF previous_date IS NOT NULL THEN
            INSERT INTO GiaMu (NgayThietLap, CongTy, GiaMuNuoc, GiaMuTap)
            VALUES (NEW.NgayGiaoDich, cong_ty, valid_gia_mu_nuoc, valid_gia_mu_tap);
            INSERT INTO TriggerLog (Message)
            VALUES (CONCAT('Added GiaMu for CongTy: ', cong_ty, ' on ', NEW.NgayGiaoDich, ' from ', previous_date));
        ELSE
            -- Ghi log và báo lỗi nếu không có giá mủ trước đó
            INSERT INTO TriggerLog (Message)
            VALUES (CONCAT('Failed to add GiaMu for CongTy: ', cong_ty, ' on ', NEW.NgayGiaoDich, ': No previous GiaMu found'));
            SIGNAL SQLSTATE '45000'
            SET MESSAGE_TEXT = 'No previous GiaMu found for the company';
        END IF;
    END IF;
    
    -- Lấy GiaMuNuoc và GiaMuTap từ bảng GiaMu cho ngày giao dịch
    SELECT GiaMuNuoc, GiaMuTap
    INTO valid_gia_mu_nuoc, valid_gia_mu_tap
    FROM GiaMu
    WHERE NgayThietLap = NEW.NgayGiaoDich
    AND CongTy = cong_ty
    LIMIT 1;
    
    -- Gán giá trị cho GiaMuNuoc và GiaMuTap
    SET NEW.GiaMuNuoc = valid_gia_mu_nuoc;
    SET NEW.GiaMuTap = valid_gia_mu_tap;
    
    -- Ghi log để xác nhận gán giá mủ
    INSERT INTO TriggerLog (Message)
    VALUES (CONCAT('Assigned GiaMuNuoc: ', valid_gia_mu_nuoc, ', GiaMuTap: ', valid_gia_mu_tap, ' for CongTy: ', cong_ty, ' on ', NEW.NgayGiaoDich));
END//

-- Trigger tính TongTien trước khi INSERT
CREATE TRIGGER before_giaodich_update_tongtien
BEFORE INSERT ON GiaoDich
FOR EACH ROW
BEGIN
    -- Kiểm tra xem tất cả các giá trị cần thiết có không NULL
    IF NEW.MuNuoc IS NOT NULL 
       AND NEW.TSC IS NOT NULL 
       AND NEW.GiaMuNuoc IS NOT NULL 
       AND NEW.MuTap IS NOT NULL 
       AND NEW.DRC IS NOT NULL 
       AND NEW.GiaMuTap IS NOT NULL THEN
        SET NEW.TongTien = (NEW.MuNuoc * NEW.TSC * NEW.GiaMuNuoc * 100) + (NEW.MuTap * NEW.DRC * NEW.GiaMuTap * 100);
    ELSE
        SET NEW.TongTien = NULL;
    END IF;
END//

-- Trigger tính TongTien trước khi UPDATE
CREATE TRIGGER before_giaodich_update_tongtien_update
BEFORE UPDATE ON GiaoDich
FOR EACH ROW
BEGIN
    -- Kiểm tra xem tất cả các giá trị cần thiết có không NULL
    IF NEW.MuNuoc IS NOT NULL 
       AND NEW.TSC IS NOT NULL 
       AND NEW.GiaMuNuoc IS NOT NULL 
       AND NEW.MuTap IS NOT NULL 
       AND NEW.DRC IS NOT NULL 
       AND NEW.GiaMuTap IS NOT NULL THEN
        SET NEW.TongTien = (NEW.MuNuoc * NEW.TSC * NEW.GiaMuNuoc) + (NEW.MuTap * NEW.DRC * NEW.GiaMuTap);
    ELSE
        SET NEW.TongTien = NULL;
    END IF;
END//

-- Trigger tự động tạo KhachHang
CREATE TRIGGER after_taikhoan_insert_khachhang
AFTER INSERT ON TaiKhoan
FOR EACH ROW
BEGIN
    IF NEW.VaiTro = 'KhachHang' THEN
        INSERT INTO KhachHang (IDTaiKhoan)
        VALUES (NEW.IDTaiKhoan);
        INSERT INTO TriggerLog (Message)
        VALUES (CONCAT('Added KhachHang for IDTaiKhoan: ', NEW.IDTaiKhoan));
    END IF;
END//

-- Trigger tự động tạo QuanLy
CREATE TRIGGER after_taikhoan_insert_quanly
AFTER INSERT ON TaiKhoan
FOR EACH ROW
BEGIN
    IF NEW.VaiTro = 'QuanLy' THEN
        INSERT INTO QuanLy (IDTaiKhoan)
        VALUES (NEW.IDTaiKhoan);
        INSERT INTO TriggerLog (Message)
        VALUES (CONCAT('Added QuanLy for IDTaiKhoan: ', NEW.IDTaiKhoan));
    END IF;
END//

-- Trigger tự động tạo QuanTri
CREATE TRIGGER after_taikhoan_insert_quantri
AFTER INSERT ON TaiKhoan
FOR EACH ROW
BEGIN
    IF NEW.VaiTro = 'QuanTri' THEN
        INSERT INTO QuanTri (IDTaiKhoan)
        VALUES (NEW.IDTaiKhoan);
        INSERT INTO TriggerLog (Message)
        VALUES (CONCAT('Added QuanTri for IDTaiKhoan: ', NEW.IDTaiKhoan));
    END IF;
END//

-- Trigger cập nhật ThanhToan
CREATE TRIGGER after_giaodich_insert
AFTER INSERT ON GiaoDich
FOR EACH ROW
BEGIN
    DECLARE thang_nam VARCHAR(7);
    DECLARE tong_mu_nuoc DECIMAL(10,2);
    DECLARE tong_mu_tap DECIMAL(10,2);
    DECLARE gia_mu_nuoc DECIMAL(10,2);
    DECLARE gia_mu_tap DECIMAL(10,2);
    
    SET thang_nam = DATE_FORMAT(NEW.NgayGiaoDich, '%Y-%m');
    SET gia_mu_nuoc = NEW.GiaMuNuoc;
    SET gia_mu_tap = NEW.GiaMuTap;

    IF EXISTS (
        SELECT 1 
        FROM ThanhToan 
        WHERE IDKhachHang = NEW.IDKhachHang 
        AND Thang = thang_nam
    ) THEN
        UPDATE ThanhToan
        SET 
            TongMuNuoc = TongMuNuoc + COALESCE(NEW.MuNuoc, 0),
            TongMuTap = TongMuTap + COALESCE(NEW.MuTap, 0),
            TongThanhToan = COALESCE(TongThanhToan, 0) + COALESCE(NEW.TongTien, 0)
        WHERE IDKhachHang = NEW.IDKhachHang 
        AND Thang = thang_nam;
    ELSE
        INSERT INTO ThanhToan (IDKhachHang, Thang, TongMuNuoc, TongMuTap, TongThanhToan)
        VALUES (
            NEW.IDKhachHang,
            thang_nam,
            COALESCE(NEW.MuNuoc, 0),
            COALESCE(NEW.MuTap, 0),
            COALESCE(NEW.TongTien, 0)
        );
    END IF;
END//

-- Stored Procedure cập nhật KhachHang
CREATE PROCEDURE UpdateKhachHangInfo(
    IN p_TenDangNhap VARCHAR(50),
    IN p_HoVaTen VARCHAR(100),
    IN p_Gmail VARCHAR(100),
    IN p_RFID VARCHAR(50),
    IN p_SoDienThoai VARCHAR(15),
    IN p_CongTy VARCHAR(100),
    IN p_SoTaiKhoan VARCHAR(50),
    IN p_NganHang VARCHAR(100)
)
BEGIN
    DECLARE v_IDTaiKhoan INT;
    SELECT IDTaiKhoan INTO v_IDTaiKhoan
    FROM TaiKhoan
    WHERE TenDangNhap = p_TenDangNhap AND VaiTro = 'KhachHang';
    
    IF v_IDTaiKhoan IS NOT NULL THEN
        UPDATE KhachHang
        SET 
            HoVaTen = p_HoVaTen,
            Gmail = p_Gmail,
            RFID = p_RFID,
            SoDienThoai = p_SoDienThoai,
            CongTy = p_CongTy,
            SoTaiKhoan = p_SoTaiKhoan,
            NganHang = p_NganHang
        WHERE IDTaiKhoan = v_IDTaiKhoan;
        INSERT INTO TriggerLog (Message)
        VALUES (CONCAT('Updated KhachHang info for TenDangNhap: ', p_TenDangNhap));
    ELSE
        INSERT INTO TriggerLog (Message)
        VALUES (CONCAT('Failed to update KhachHang: TenDangNhap ', p_TenDangNhap, ' not found or not KhachHang'));
    END IF;
END//

-- Stored Procedure cập nhật QuanLy
CREATE PROCEDURE UpdateQuanLyInfo(
    IN p_TenDangNhap VARCHAR(50),
    IN p_HoVaTen VARCHAR(100),
    IN p_SoDienThoai VARCHAR(15),
    IN p_Gmail VARCHAR(100),
    IN p_CongTy VARCHAR(100)
)
BEGIN
    DECLARE v_IDTaiKhoan INT;
    SELECT IDTaiKhoan INTO v_IDTaiKhoan
    FROM TaiKhoan
    WHERE TenDangNhap = p_TenDangNhap AND VaiTro = 'QuanLy';
    
    IF v_IDTaiKhoan IS NOT NULL THEN
        UPDATE QuanLy
        SET 
            HoVaTen = p_HoVaTen,
            SoDienThoai = p_SoDienThoai,
            Gmail = p_Gmail,
            CongTy = p_CongTy
        WHERE IDTaiKhoan = v_IDTaiKhoan;
        INSERT INTO TriggerLog (Message)
        VALUES (CONCAT('Updated QuanLy info for TenDangNhap: ', p_TenDangNhap));
    ELSE
        INSERT INTO TriggerLog (Message)
        VALUES (CONCAT('Failed to update QuanLy: TenDangNhap ', p_TenDangNhap, ' not found or not QuanLy'));
    END IF;
END//

-- Stored Procedure cập nhật QuanTri
CREATE PROCEDURE UpdateQuanTriInfo(
    IN p_TenDangNhap VARCHAR(50),
    IN p_HoVaTen VARCHAR(100),
    IN p_SoDienThoai VARCHAR(15),
    IN p_Gmail VARCHAR(100)
)
BEGIN
    DECLARE v_IDTaiKhoan INT;
    SELECT IDTaiKhoan INTO v_IDTaiKhoan
    FROM TaiKhoan
    WHERE TenDangNhap = p_TenDangNhap AND VaiTro = 'QuanTri';
    
    IF v_IDTaiKhoan IS NOT NULL THEN
        UPDATE QuanTri
        SET 
            HoVaTen = p_HoVaTen,
            SoDienThoai = p_SoDienThoai,
            Gmail = p_Gmail
        WHERE IDTaiKhoan = v_IDTaiKhoan;
        INSERT INTO TriggerLog (Message)
        VALUES (CONCAT('Updated QuanTri info for TenDangNhap: ', p_TenDangNhap));
    ELSE
        INSERT INTO TriggerLog (Message)
        VALUES (CONCAT('Failed to update QuanTri: TenDangNhap ', p_TenDangNhap, ' not found or not QuanTri'));
    END IF;
END//

CREATE TRIGGER after_giaodich_update
AFTER UPDATE ON GiaoDich
FOR EACH ROW
BEGIN
    DECLARE thang_nam VARCHAR(7);
    
    -- Chỉ xử lý nếu MuNuoc, MuTap hoặc TongTien thay đổi
    IF OLD.MuNuoc <=> NEW.MuNuoc 
       AND OLD.MuTap <=> NEW.MuTap 
       AND OLD.TongTien <=> NEW.TongTien THEN
        -- Không làm gì nếu không có thay đổi
        INSERT INTO TriggerLog (Message)
        VALUES (CONCAT('No relevant changes in GiaoDich ID: ', NEW.IDGiaoDich, ' for ThanhToan update'));
    ELSE
        -- Xác định tháng-năm từ NgayGiaoDich
        SET thang_nam = DATE_FORMAT(NEW.NgayGiaoDich, '%Y-%m');

        -- Ghi log để theo dõi giá trị cập nhật
        INSERT INTO TriggerLog (Message)
        VALUES (CONCAT('Processing UPDATE for GiaoDich ID: ', NEW.IDGiaoDich, 
                       ', MuNuoc: ', COALESCE(NEW.MuNuoc, 0), 
                       ', MuTap: ', COALESCE(NEW.MuTap, 0), 
                       ', TongTien: ', COALESCE(NEW.TongTien, 0)));

        -- Kiểm tra xem bản ghi ThanhToan đã tồn tại chưa
        IF EXISTS (
            SELECT 1 
            FROM ThanhToan 
            WHERE IDKhachHang = NEW.IDKhachHang 
            AND Thang = thang_nam
        ) THEN
            -- Cập nhật bản ghi ThanhToan
            UPDATE ThanhToan
            SET 
                TongMuNuoc = TongMuNuoc - COALESCE(OLD.MuNuoc, 0) + COALESCE(NEW.MuNuoc, 0),
                TongMuTap = TongMuTap - COALESCE(OLD.MuTap, 0) + COALESCE(NEW.MuTap, 0),
                TongThanhToan = TongThanhToan - COALESCE(OLD.TongTien, 0) + COALESCE(NEW.TongTien, 0)
            WHERE IDKhachHang = NEW.IDKhachHang 
            AND Thang = thang_nam;

            -- Ghi log sau khi cập nhật
            INSERT INTO TriggerLog (Message)
            VALUES (CONCAT('Updated ThanhToan for IDKhachHang: ', NEW.IDKhachHang, 
                           ', Thang: ', thang_nam, 
                           ', New MuNuoc: ', COALESCE(NEW.MuNuoc, 0), 
                           ', New MuTap: ', COALESCE(NEW.MuTap, 0), 
                           ', New TongTien: ', COALESCE(NEW.TongTien, 0)));
        ELSE
            -- Thêm bản ghi mới vào ThanhToan
            INSERT INTO ThanhToan (IDKhachHang, Thang, TongMuNuoc, TongMuTap, TongThanhToan)
            VALUES (
                NEW.IDKhachHang,
                thang_nam,
                COALESCE(NEW.MuNuoc, 0),
                COALESCE(NEW.MuTap, 0),
                COALESCE(NEW.TongTien, 0)
            );

            -- Ghi log sau khi thêm mới
            INSERT INTO TriggerLog (Message)
            VALUES (CONCAT('Inserted ThanhToan for IDKhachHang: ', NEW.IDKhachHang, 
                           ', Thang: ', thang_nam, 
                           ', MuNuoc: ', COALESCE(NEW.MuNuoc, 0), 
                           ', MuTap: ', COALESCE(NEW.MuTap, 0), 
                           ', TongTien: ', COALESCE(NEW.TongTien, 0)));
        END IF;
    END IF;
END//


DELIMITER ;

-- Chèn dữ liệu mẫu

-- 1 tài khoản Quản trị
INSERT INTO TaiKhoan (TenDangNhap, MatKhau, NgayTao, VaiTro) VALUES
('0867688330', 'ntd123', '2025-05-023', 'QuanTri');

CALL UpdateQuanTriInfo('0867688330', 'Ngô Thành Đạt', '0867688330', 'ntdat2853@gmail.com');

ALTER USER 'root'@'%' IDENTIFIED BY '123abc';

GRANT ALL PRIVILEGES ON *.* TO 'root'@'%' WITH GRANT OPTION;
FLUSH PRIVILEGES;
