// ChirpStack: Device profile > Codec > JavaScript.
function decodeUplink(input) {
  var b = input.bytes;
  if (input.fPort !== 10 || !((b[0] === 1 && b.length === 6) ||
      (b[0] === 2 && b.length === 11)))
    return { errors: ["Expected FPort 10: v1/6 bytes or v2/11 bytes"] };
  var data = {
    version: b[0], inputs_raw: b[1],
    sequence: (b[2] * 16777216 + b[3] * 65536 + b[4] * 256 + b[5]),
    di1: !!(b[1] & 1), di2: !!(b[1] & 2),
    di3: !!(b[1] & 4), di4: !!(b[1] & 8),
    di5: !!(b[1] & 16), di6: !!(b[1] & 32),
    di7: !!(b[1] & 64), di8: !!(b[1] & 128)
  };
  if (b[0] === 2) {
    data.sensor_status = b[6];
    data.sensor_ok = b[6] === 1;
    if (data.sensor_ok) {
      var t = b[7] * 256 + b[8];
      if (t >= 32768) t -= 65536;
      data.temperature = t / 10;
      data.humidity = (b[9] * 256 + b[10]) / 10;
      if (data.temperature < -40 || data.temperature > 80 || data.humidity > 100)
        return { errors: ["DHT22 values outside valid range"] };
    }
  }
  return { data: data };
}
