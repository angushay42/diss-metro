def validate(psc):
    return 0 <= psc <= 65535 

def main():
    apb = 84_000_000
    max_psc = 65535

    # frequency of led pulse
    fpulse = 1 / (100 / 1000)
    min_bpm = 40 / 60
    max_bpm = 220 / 60

    psc = 0
    clk = 0
    # start with 1% of max psc
    mul = 0.01
    while True:
        psc = max_psc * mul
        clk = apb / psc
        if (validate(clk / fpulse) 
            and validate(clk / min_bpm) 
            and validate(clk / max_bpm)
        ):
            break
        mul += 0.01

    print(f"clk frequency: {clk}, psc: {psc}")
    print(f"pscs: LED: {clk / fpulse}, min: {clk / min_bpm}, max: {clk / max_bpm}")
    

if __name__ == "__main__":
    main()