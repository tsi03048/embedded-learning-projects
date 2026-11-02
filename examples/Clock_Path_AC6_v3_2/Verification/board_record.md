# B02 보드 기록 — 전 항목 미실행

- 실물Part·BoardRevision·8MHz크리스털 표기·전원·Target/AXF해시·Compiler6.22:
- 01 baseline의HSIRDY·SW/SWS·HPRE/PPRE: 미실행
- 01 apply_request1 후PPRE1=100·SWS=00·HCLK유지·PCLK1분주: 미실행
- 02 HSEON/HSERDY·SW=01/SWS=01·stage50/error0: 미실행
- 03 FLASH관심비트0x12·HSE/PLLready·SW=10/SWS=10·분주/배수·stage50/error0: 미실행
- HSE준비·PLLlock·전환상태를 각각 관찰한 지점·실측시간: 미실행
- SystemCoreClock·계산Hz와 실제측정 주파수는 별도 열에 기록
- 실패 시error/stage/final/live CR·CFGR, SW요청 전후 구분: 미실행
- 선택MCO 측정: PA8공유확인·AF설정·출력소스·50MHz한계·프로브/장비·실제Hz: 미실행

poll_budget은횟수다. 값을바꿔 소프트웨어관찰기한을시험한 결과를실제크리스털불량·물리안정화시간으로표현하지않는다.
