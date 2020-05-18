/**
  @author Hidden Track
  @since 2020/05/18
  @time: 12:01
 */


#include "SoftlyNetTest.h"
#include "seeker/logger.h"

#include <iostream>
#include <memory>

using std::cout;
using std::endl;
using std::shared_ptr;


class A {
 public:
  int i;
  A(int n) : i(n){};
  ~A() {
    cout << i << " "
         << "destructed" << endl;
  }
};
int main() {
  seeker::Logger::init();
  shared_ptr<A> sp1(new A(2));  // A(2)由sp1托管，
  shared_ptr<A> sp2(sp1);       // A(2)同时交由sp2托管
  shared_ptr<A> sp3;
  sp3 = sp2;  // A(2)同时交由sp3托管
  cout << sp1->i << "," << sp2->i << "," << sp3->i << endl;
  A* p = sp3.get();        // get返回托管的指针，p 指向 A(2)
  cout << p->i << endl;    //输出 2
  sp1.reset(new A(3));     // reset导致托管新的指针, 此时sp1托管A(3)
  sp2.reset(new A(4));     // sp2托管A(4)
  cout << sp1->i << endl;  //输出 3
  sp3.reset(new A(5));     // sp3托管A(5),A(2)无人托管，被delete
  cout << "end" << endl;
  return 0;
}
