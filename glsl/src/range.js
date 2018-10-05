const IGNORE = 0xff;
const EXCLUDE_BOTH = 0x00;
const INCLUDE_LEFT = 0x01;
const INCLUDE_RIGHT = 0x02;
const INCLUDE_BOTH = 0x03;

//todo: tests
export default class Range {
    constructor(left, right, flag) {
        this.left = left;
        this.right = right;
        this.flag = flag;
    }

    get includeLeft() { return this.flag & INCLUDE_LEFT; }
    get includeRight() { return this.flag & INCLUDE_RIGHT; }
    get nonFloat() { return this.flag === IGNORE; }
    toString() {
        if (this.nonFloat) return "[?]";

        let out = [];
        out.push(this.includeLeft ? "[" : "(");
        out.push(f2s(this.left));
        if (this.left !== this.right) {
            out.push(",");
            out.push(f2s(this.right));
        }
        out.push(this.includeRight ? "]" : ")");
        return out.join("");
    }

    static create(type, rangeAsStr) {
        if (type === "boolean") return new Range(0, 1, INCLUDE_BOTH);
        if (type !== "float" && type !== "integer") return Range.none;
        if (typeof rangeAsStr === "number") return new Range(rangeAsStr, rangeAsStr, INCLUDE_BOTH);
        if (!rangeAsStr) return new Range(Number.NEGATIVE_INFINITY, Number.POSITIVE_INFINITY, INCLUDE_BOTH);
        let [left,right] = rangeAsStr.split(",");
        let flag = EXCLUDE_BOTH;
        if (left[0] === "[") {
            flag |= INCLUDE_LEFT;
            left = parseFloat(left.substring(1));
        } else if (left[0] === "(") {
            left = parseFloat(left.substring(1));
        } else {
            flag |= INCLUDE_LEFT;
            left = parseFloat(left);
        }
        if (right !== undefined) {
            if (right[right.length-1] !== ")") {
                flag |= INCLUDE_RIGHT;
            }
            right = parseFloat(right);
        } else {
            right = left;
            flag |= INCLUDE_RIGHT;
        }
        if (isNaN(left) || isNaN(right)) return new Range(Number.NEGATIVE_INFINITY, Number.POSITIVE_INFINITY, INCLUDE_BOTH);
        return new Range(left, right, flag);
    }
}

function f2s(num) {
    if (!isFinite(num)) {
        return (num > 0 ? "∞" : "-∞");
    }
    let str1 = num.toFixed(3);
    let str2 = num.toString();
    return str1.length < str2.length ? str1 : str2;
}

Range.none = new Range(-12345,12345, IGNORE);

window.createRange = function(left, right, flag) {
    return new Range(left, right, flag);
}