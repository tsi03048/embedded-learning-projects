# B01 보드 관찰 기록 — 현재 미실행

- 실물 Part / 보드 Revision / 전원 경로·점퍼 / Target·AXF해시 / Compiler6.22·옵션:
- 01 처음 capture_done1: boot_csr·6개 원인·다운로드/디버그 방식 / 현재 미실행
- 01 clear_request1 후 clear_done1: 저장한boot_csr와after_clear_flags / 현재 미실행
- 01 하드웨어NRST 후 재실행: clear_request·clear_done초기화, 새boot_csr·PINRSTF / 현재 미실행
- 02 정상전원: 실제VDD/VDDA·pvd_cr·pvd_csr·pvdo·pvd_ready / 현재 미실행
- 02 전압변화: 공급방법·역급전제거·허용조건확인, 하강/상승경계·파형·관찰위치 / 현재 미실행
- 전원Ramp·NRST해제→실행시각: 실측장비·관측지점·별도계획 / 현재 미실행

전원을 움직이는 동안 중단점 관찰값을 실시간 파형으로 취급하지 않는다. PVDO0만으로 이전저전압사건이없었다고판정하지않는다.
