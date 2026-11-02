E06 PC 실습 묶음 v3.2

기존 10개 C 소스를 수정하지 않고 모았습니다. STM32 펌웨어가 아니라 Windows PC의 가상 기록·계산·요청 정책 실습입니다.
준비: Visual Studio의 C++ Build Tools를 설치한 PC에서 x64 Developer Command Prompt를 엽니다.
압축을 푼 이 폴더로 이동해 run_all.cmd를 실행하면 /W4 /WX /TC로 각각 빌드하고 검사합니다.
각 폴더의 C 파일만 따로 빌드해도 됩니다. 결과 exe/obj는 build에 생성합니다. ARM/Keil 실행 파일을 PC에서 실행하지 않습니다.

먼저 예상 → 해당 파일의 main 호출과 함수 몸체 → 실행 출력의 got/expected 또는 PASS/FAIL → Notion 비교표 순서로 봅니다.
일반 경로: 02→04→06→08→10→12→19→21. 15·17은 D04 뒤의 모터 적용 가상 예제입니다.
이 묶음은 소스를 새로 함수화한 예제가 아닙니다. 원래 각 페이지에 공개된 전체 C의 선언·구현·호출을 그대로 포함합니다.
시간/출처를 가상 입력으로 줍니다. 실제 ADC, 타이머, 인터럽트, 센서 또는 모터를 실행하거나 검증하지 않습니다.
10개 프로그램은 합계98개 check를 실행합니다. 06의 65,536조합과 08의10,000조합은 각각 한 check 안에서 반복 확인합니다.
배포본 소스·원래 ZIP·기존 페이지는 보존합니다. 실제 보드 검증은 별도입니다.

파일별 예상:
E06-02: 02_fresh_samples/fresh_samples.c — 12/12 checks passed (fresh/watch는 passed)
E06-04: 04_periodic_watch/watch.c — 13/13 checks passed (fresh/watch는 passed)
E06-06: 06_counter_wrap/counter_wrap.c — 8/8 checks passed (fresh/watch는 passed)
E06-08: 08_counter_100/counter_100.c — 8/8 checks passed (fresh/watch는 passed)
E06-10: 10_extended_counter/extended_counter.c — 8/8 checks passed (fresh/watch는 passed)
E06-12: 12_timer_read_race/timer_read_race.c — 5/5 checks passed (fresh/watch는 passed)
E06-15: 15_sensorless_review/review.c — 13/13 checks passed (fresh/watch는 passed)
E06-17: 17_step_identity/step_identity.c — 13/13 checks passed (fresh/watch는 passed)
E06-19: 19_sample_origin/sample_origin.c — 8/8 checks passed (fresh/watch는 passed)
E06-21: 21_request_busy/request_busy.c — 10/10 checks passed (fresh/watch는 passed)
