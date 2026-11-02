# D-E07-01 — 의도적으로 잘못 붙인 출처의 요구 판정

범위: E06-19 sample_origin.c의 결정적 PC 시연. 새로 발견한 제품 결함이 아니다.
요구: REQ-E07-02. A(1/W) 결과는 현재 B(2/V)의 새 결과로 인정하지 않는다.
재현: 원본 main은 correct에 A 출처를 저장한다. wrong.origin=step2와 wrong.value=correct.value로 잘못된 결과를 별도로 구성한다.
관찰: wrong label matches current: PASS / wrong label hides origin: PASS.
해석: 잘못된 문맥을 붙이면 현재 B와 같아져 버리는 현상을 재현한 check가 PASS이다. wrong 분기는 요구를 FAIL한다.
원인: 값은 A에서 얻었지만 출처를 완료 시점의 B로 바꾸었다.
비교: 원본 complete의 result->origin=request->origin을 쓰는 correct는 B와 불일치한다. 원본에 이미 두 경로가 있어 소스 수정은 하지 않았다.
재시험: 실제로 코드를 고쳤다면 새 SHA/Commit으로 T-FAULT-01과 T-PC-01~04를 다시 실행한다. 이번 두 실행은 기존 경로 비교이며 수정 후 재시험 실적이 아니다.
남은 검증: 실제 ADC 트리거 수락·결과와 메타데이터의 대응·동시 실행은 미검증.
