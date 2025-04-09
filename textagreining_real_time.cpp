#include <iostream>
#include <opencv2/opencv.hpp>
#include <tesseract/baseapi.h>
#include <leptonica/allheaders.h>
#include <ctime>
#include <cstdlib>
#include <cstring>

int main() {
    // Opna myndavél með OpenCV (gert ráð fyrir að libcamera sé tengt við /dev/video0)
    cv::VideoCapture cap(0);
    if (!cap.isOpened()) {
        std::cerr << "Gat ekki opnað myndavélina.\n";
        return -1;
    }

    // Setja upplausn (ef libcamera styður)
    cap.set(cv::CAP_PROP_FRAME_WIDTH, 640);
    cap.set(cv::CAP_PROP_FRAME_HEIGHT, 480);

    // Bíða smá meðan myndavélin "vaknar"
    cv::Mat temp;
    for (int i = 0; i < 5; ++i) {
        cap >> temp;
        cv::waitKey(30);
    }

    // Tesseract OCR stillingar
    tesseract::TessBaseAPI *ocr = new tesseract::TessBaseAPI();
    if (ocr->Init(NULL, "eng")) {
        std::cerr << "Tesseract tókst ekki að frumstilla.\n";
        return -1;
    }

    std::string lastSpokenText = "";
    std::time_t lastReadTime = 0;
    const int cooldownSeconds = 2;

    cv::Mat frame;
    while (true) {
        cap >> frame;
        if (frame.empty()) continue;

        // Umbreyta yfir í grátóna
        cv::Mat gray;
        cv::cvtColor(frame, gray, cv::COLOR_BGR2GRAY);

        // OCR – lesa texta úr myndinni
        ocr->SetImage(gray.data, gray.cols, gray.rows, 1, gray.step);
        char *text_cstr = ocr->GetUTF8Text();
        if (!text_cstr) continue;

        std::string text(text_cstr);
        delete[] text_cstr;

        // Fjarlægja línuskil og hvít bil
        text.erase(std::remove(text.begin(), text.end(), '\n'), text.end());
        text.erase(std::remove_if(text.begin(), text.end(), ::isspace), text.end());

        std::time_t now = std::time(nullptr);
        if (!text.empty() && text != lastSpokenText && std::difftime(now, lastReadTime) >= cooldownSeconds) {
            std::cout << "Detected text:\n" << text << std::endl;

            // Búa til espeak-ng skipun og keyra hana
            std::string command = "espeak-ng \"" + text + "\"";
            system(command.c_str());

            lastSpokenText = text;
            lastReadTime = now;
        }

        // Sýna mynd
        cv::imshow("Camera", frame);

        // Ef ýtt á 'q' – hætta
        char key = (char)cv::waitKey(1);
        if (key == 'q') break;
    }

    // Hreinsa
    ocr->End();
    delete ocr;
    cap.release();
    cv::destroyAllWindows();

    return 0;
}
