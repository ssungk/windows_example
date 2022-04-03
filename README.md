# Windows Example
ssungk's winsows example repository


## Environment
OS: Windows 10
Compiler : visual studio 2017

## Build & Test
특별한 작업없이 visual studio를 이용해 빌드해주시면 됩니다.  
visual studio통해 실행시 바이너리가 sln파일 위치에 생성됩니다. 


## 01_desktop_duplication_api
* 윈도우에서 제공하는 Desktop Duplication API를 이용한 화면캡처 및 bgr포맷의 RAW데이터로 저장하는 예제
* 이전에 제공하던 GDI를 이용한 화면 캡처 방식보다 발전된 방식
* 화면캡처를 위한 AP이때 때문에 GPU메모리에서 접근이 가능해 속도가 매우빠르고 GPU가속 인코딩과 조합하면 효율이 좋음