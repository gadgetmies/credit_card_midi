void setup() {
  pinMode(CM_MIXER_LED_CUE_L, OUTPUT);
  pinMode(CM_MIXER_LED_CUE_R, OUTPUT);
}

void loop() {
  digitalWrite(CM_MIXER_LED_CUE_L, HIGH);
  digitalWrite(CM_MIXER_LED_CUE_R, HIGH);
  delay(1000);
  digitalWrite(CM_MIXER_LED_CUE_L, LOW);
  digitalWrite(CM_MIXER_LED_CUE_R, LOW);
  delay(1000);
}
