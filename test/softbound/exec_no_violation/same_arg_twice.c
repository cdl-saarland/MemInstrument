// RUN: %clang -Xclang -load -Xclang %passlib -O1 %s -mllvm -mi-config=softbound %linksb -o %t
// RUN: %t

float GA[16];
float GB[16];

#define A(ar,row,col)  ar[(col<<2)+row]

void matmul(float *res, float *first, float *second) {
    for (int i = 0; i < 4; i++) {
        float firsti0 = A(first, i, 0);
        float firsti1 = A(first, i, 1);
        float firsti2 = A(first, i, 2);
        float firsti3 = A(first, i, 3);
        A(res, i, 0) = firsti0 * A(second, 0, 0) + firsti1 * A(second, 1, 0) + firsti2 * A(second, 2, 0) + firsti3 * A(second, 3, 0);
        A(res, i, 1) = firsti0 * A(second, 0, 1) + firsti1 * A(second, 1, 1) + firsti2 * A(second, 2, 1) + firsti3 * A(second, 3, 1);
        A(res, i, 2) = firsti0 * A(second, 0, 2) + firsti1 * A(second, 1, 2) + firsti2 * A(second, 2, 2) + firsti3 * A(second, 3, 2);
        A(res, i, 3) = firsti0 * A(second, 0, 3) + firsti1 * A(second, 1, 3) + firsti2 * A(second, 2, 3) + firsti3 * A(second, 3, 3);
    }
}

int main() {
    matmul(GA, GA, GB);
    return 0;
}
