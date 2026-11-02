# B03 보드 비교 기록 — 전 항목 미실행

- 실물Part·BoardRevision·전원·Target/소스/AXF해시·AC6.22·-O0·디버그freeze설정:
- 01 phase1: ENR bit0=1 / CEN0 / PSC7999 / ARR9999 / CNT0 / initUIF와Clear: 미실행
- 01 request1→phase2: CEN1·Run중CNT변화: 미실행
- 01 request2→phase3: PPRE1=101, PSC/ARR유지, 명목TIM2입력4MHz·계수500Hz와실제관측구분: 미실행
- 02 phase1: CEN0·CNT1234·PSC7999·ARR9999: 미실행
- 02 phase2: TIM2EN0·RAM준비복사본만기록, TIM2레지스터읽지않음: 미실행
- 02 phase3: TIM2EN1·재활성뒤같은네필드비교: 미실행
- 02 phase4: TIM2RST1·TIM2EN1·TIM2레지스터읽지않음: 미실행
- 02 phase5: TIM2RST0·TIM2EN1·CR1=0/CNT0/PSC0/ARR65535·main진행유지: 미실행
- gate전후소비전류·계수주파수·동작중gate정지시간은별도조건·계측계획: 미실행
