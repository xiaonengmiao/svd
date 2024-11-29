#include <iostream>
#include <Eigen/Dense>

int main()
{
    Eigen::Matrix<float, 4, 5> A {
      {1, 0, 0, 0, 2},
      {0, 0, 3, 0, 0},
      {0, 0, 0, 0, 0},
      {0, 2, 0, 0, 0},
    };
    std::cout << "A: " << A << std::endl;

    Eigen::JacobiSVD<Eigen::MatrixXf> svd(A, Eigen::ComputeFullU | Eigen::ComputeFullV);
    std::cout << "U: " << svd.matrixU() << std::endl;
    std::cout << "V: " << svd.matrixV() << std::endl;
}