var color = Chart.helpers.color;
var @variable# = {
labels: [@category#],
datasets: [{
label: @label2#,
backgroundColor: color(window.chartColors.yellow).alpha(0.5).rgbString(),
borderColor: window.chartColors.yellow,
borderWidth: 1,
data: [
@warning#
]
}, {
label: @label3#,
backgroundColor: color(window.chartColors.red).alpha(0.5).rgbString(),
borderColor: window.chartColors.red,
borderWidth: 1,
data: [
@critical#
]
}, {
label: @label4#,
backgroundColor: color(window.chartColors.purple).alpha(0.5).rgbString(),
borderColor: window.chartColors.purple,
borderWidth: 1,
data: [
@block#
]
}]
};
