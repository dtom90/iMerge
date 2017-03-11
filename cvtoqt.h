#ifndef CVTOQT_H
#define CVTOQT_H

#include <QImage>
#include <opencv2/core/core.hpp>

using namespace cv;

QImage putImage(const Mat& mat);

#endif // CVTOQT_H
