# Windows Example
ssungk's winsows example repository  
windows코드 기록용 git repository


## Development Environment
OS: Windows 10  
Compiler : visual studio 2017

## Build
특별한 작업없이 visual studio를 이용해 빌드해주시면 됩니다.  
visual studio통해 실행시 결과 데이터가 sln파일 위치에 생성됩니다.

## Test
Tool에 포함된 프로그램을 통해 결과를 확인할수있습니다  
예제1의 결과물은 bgr이고 YUV Player는 rgb포맷만 지원하기 때문에  
변질된 색으로 데이터만 가능하고 원본 그대로 재생은 불가능합니다.   
1,2,5번 예제 : YUV Player를 통해 확인해주세요  
3,4번 예제 : h264 분석기로 데이터 확인 or 팟플레이어로 재생해 확인 가능합니다.


## 01 Desktop Duplication API
* 윈도우에서 제공하는 Desktop Duplication API를 이용한 화면캡처 및 bgr포맷의 RAW데이터로 저장하는 예제
* 이전에 제공하던 GDI를 이용한 화면 캡처 방식보다 발전된 방식
* 화면캡처를 위한 AP이때 때문에 GPU메모리에서 접근이 가능해 속도가 매우빠르고 GPU가속 인코딩과 조합하면 효율이 좋음

## 02 Media foundation Video Processor
* 윈도우에서 제공하는 Media foundation API를 이용한 bgr to nv12(yuv)예제
* SW, HW둘다 지원하지만 HW의 경우 DirectX를 사용해야되서 SW예제로 구현
* 1번 예제와 달리 다중모니터를 고려하지 않고 주모니터1개만 동작
* 개인적으로 Video Processor 보다는 DirectX의 쉐이더 기능 쓰는것을 추천

## 03 Media foundation SW Encoder
* 윈도우에서 제공하는 Media foundation API를 이용한 nv12 to h264 예제
* Video Processor와 달리 output type이 먼저 설정되어야함
* 몇가지 동작(SW로 동작, h264코덱으로만 동작 등...)을 가정하고 구현했기에 상용제품이 사용시 좀더 예외처리가 필요함

## 04 Media foundation HW Encoder
* 윈도우에서 제공하는 Media foundation API의 하드웨어 가속(그래픽카드)을 통한 nv12 to h264 예제
* 하드웨어 인코더의 경우 무조건 asnyc로 동작
* 예제에서는 async로 처리를 위한 멀티쓰레딩과 mutex사용을 자제하기위해 future promise를 통해 blocking동작으로 처리
* 실제 제품에 동작하기위해서는 hw와 sw동작 둘다 고려 멀티쓰레딩 등 고려하고 예외처리 해야할 부분이 많음
* 4번예제는 엔비디아, 인텔에 테스트 해봤으나 컴퓨터 하드웨어 따라 동작 안할수도 있음(하드웨어따라 동작이 미묘하게 다른게 있음)

## 05 Media foundation SW Decoder
* 윈도우에서 제공하는 Media foundation AP를 이용한 h264 to yuv420 예제
* 인코딩때 입력으로 nv12를 사용한것과 달리 디코딩은 yuv420으로 출력하게 함
