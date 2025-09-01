Scanning Tools by Mrx_112

Sebuah alat pemindaian jaringan yang ditulis dalam C++ untuk melakukan resolusi domain, pencarian lokasi server, dan pemindaian port.
📋 Fitur

    Resolusi Domain: Mengonversi nama domain ke alamat IP

    GeoIP Lookup: Mendapatkan informasi lokasi server (negara, kota, ISP, dll.)

    Pemindaian Port: Memeriksa port umum yang terbuka pada server target

    Multi-endpoint Support: Menggunakan beberapa layanan GeoIP (ip-api.com, ipapi.co, ipinfo.io)

🛠️ Teknologi

    Bahasa Pemrograman: C++

    Library: libcurl (untuk HTTP requests), socket API (untuk koneksi jaringan)

    Sistem Operasi: Linux/Unix

📦 Instalasi
Prerequisites

Pastikan Anda telah menginstal library yang diperlukan:
bash

# Ubuntu/Debian
sudo apt-get install libcurl4-openssl-dev build-essential

# CentOS/RHEL
sudo yum install libcurl-devel gcc-c++

Kompilasi
bash

g++ -o scanning_tools scanning_tools.cpp -lcurl

🚀 Penggunaan

    Jalankan program yang telah dikompilasi:

bash

./scanning_tools

    Masukkan nama website ketika diminta (contoh: example.com)

    Program akan menampilkan:

        Alamat IP dari domain

        Informasi lokasi server (jika tersedia)

        Status port-port umum

📊 Port yang Diperiksa

Program memeriksa port-port umum berikut:

    80 (HTTP)

    443 (HTTPS)

    8080 (HTTP Alternate)

    8443 (HTTPS Alternate)

    21 (FTP)

    22 (SSH)

    25 (SMTP)

    110 (POP3)

    143 (IMAP)

    3306 (MySQL)

    3389 (RDP)

⚠️ Catatan

    Program ini memerlukan koneksi internet untuk bekerja

    Beberapa server mungkin menggunakan perlindungan (CDN/Cloudflare) yang menyembunyikan informasi lokasi sebenarnya

    Pemindaian port mungkin terdeteksi sebagai aktivitas mencurigakan oleh beberapa penyedia layanan

📝 Lisensi

Proyek ini dibuat oleh Mrx_112. Silakan modifikasi dan distribusikan sesuai kebutuhan.
🤝 Kontribusi

Silakan fork repository ini dan ajukan pull request untuk perbaikan atau fitur tambahan.
