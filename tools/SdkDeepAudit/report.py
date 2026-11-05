import argparse
import collections
import gzip
import html
import json
from pathlib import Path


def read(path):
    return json.loads(path.read_text(encoding='utf-8-sig'))


def summarize(output):
    manifest = read(output / 'run.json')
    summaries, examples = [], []
    layout_kinds = {'record_size', 'record_alignment', 'offsetBits', 'bitWidth', 'type_size', 'type_alignment'}
    for configuration in manifest['configurations']:
        key = configuration['architecture'] + '-' + configuration['characterSet']
        folder = output / 'reports' / key
        summary = read(folder / 'summary.json')
        summaries.append(summary)
        selected = collections.Counter()
        with gzip.open(folder / 'findings.jsonl.gz', 'rt', encoding='utf-8') as stream:
            for line in stream:
                row = json.loads(line)
                important = [item for item in row['differences'] if item['kind'] in layout_kinds]
                if row['status'] != 'different' or not important:
                    continue
                bucket = important[0]['kind']
                if selected[bucket] >= 20:
                    continue
                selected[bucket] += 1
                source = row.get('sdk_candidates', [{}])[0].get('site', {})
                examples.append({'configuration': key, 'name': row['name'], 'status': row['status'], 'differences': important, 'projection': row['projection']['site'], 'sdk': source})
    result = {'configurations': summaries, 'all_symbols_verified': all(item['all_symbols_verified'] for item in summaries), 'layout_examples': examples}
    (output / 'summary.json').write_text(json.dumps(result, ensure_ascii=False, indent=2), encoding='utf-8')
    rows = []
    for summary in summaries:
        states = summary['statuses']
        passed = sum(value for name, value in states.items() if name.startswith('matched'))
        categories = summary.get('symbols_by_difference_category', {})
        report = 'reports/' + summary['configuration'] + '/'
        rows.append('<tr>' + ''.join('<td>' + str(value) + '</td>' for value in (summary['configuration'], summary['symbols'], summary['parameters'], passed, states.get('different', 0), categories.get('layout', 0), states.get('unmatched', 0), states.get('ambiguous', 0), summary['failed_or_tainted_translation_units'])) + '<td><a href="' + report + 'differences.csv">逐项差异</a> · <a href="' + report + 'findings.csv">全部待查项</a> · <a href="' + report + 'symbols.jsonl.gz">完整符号记录</a></td></tr>')
    data = json.dumps(examples, ensure_ascii=False).replace('</', '<\\/')
    page = '''<!doctype html><html lang="zh-CN"><meta charset="utf-8"><title>SDK AST 深度自校验</title>
<style>body{font:15px/1.6 system-ui;margin:32px;color:#182435;background:#f6f8fb}h1{font-size:28px}table{border-collapse:collapse;background:white;width:100%}td,th{padding:9px 12px;border:1px solid #dce2ea;text-align:left}th{background:#eaf0f7}code{font:13px Consolas,monospace;overflow-wrap:anywhere}a{color:#075dad}input,select{padding:9px;font:inherit;margin:8px 12px 16px 0}.note{background:#fff3cd;padding:16px;border-left:4px solid #d1a000}.item{background:white;margin:12px 0;padding:16px;border:1px solid #dce2ea}pre{white-space:pre-wrap}.path{font-size:13px;color:#506077}</style>
<h1>SDK AST 深度自校验</h1><p>双侧均使用本机 ClangSharp 的 libclang 解析。此报告记录确定差异、允许的枚举替换、宏和 UUID 对应关系，以及无法验证的项目。</p>
<p class="note">差异统计不等于已确认的导出缺陷数量：类型写法、C++ noexcept、宏表达式类型大小及有符号数表示会分别统计。解析失败、同名歧义、未匹配及无法求值均不会算作通过。不同配置和不同类别之间可能包含同一个逻辑符号。</p>
<table><thead><tr><th>配置</th><th>声明</th><th>参数</th><th>已对应并验证</th><th>有差异的声明</th><th>涉及布局的声明</th><th>未匹配</th><th>歧义</th><th>解析有误的单元</th><th>详细记录</th></tr></thead><tbody>''' + ''.join(rows) + '''</tbody></table>
<p>字段偏移以位记录；下面的示例同时显示字节偏移。Macro 表达式没有独立存储对象，其类型大小与导出常量对象大小不同，需要结合枚举配置审核。函数机器码长度、链接后地址和指令不在头文件 AST 的验证范围内。</p>
<h2>布局差异示例</h2><p>每种检查项、每个配置最多展示 20 个示例，完整数据请查看上表中的 CSV 和 JSONL。</p><input id="query" placeholder="按符号或文件筛选"><select id="configuration"><option value="">所有配置</option></select><div id="items"></div>
<script>const data=''' + data + ''';
const q=document.querySelector('#query'),c=document.querySelector('#configuration'),out=document.querySelector('#items');
const esc=x=>String(x).replace(/[&<>"']/g,s=>({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;',"'":'&#39;'}[s]));
for(const name of [...new Set(data.map(x=>x.configuration))]){let o=document.createElement('option');o.value=o.textContent=name;c.appendChild(o)}
function value(x,k){return k==='offsetBits'&&typeof x==='number'?x+' 位（字节 '+Math.floor(x/8)+'，位内 '+x%8+'）':JSON.stringify(x)}
function render(){out.innerHTML=data.filter(x=>(!c.value||x.configuration===c.value)&&JSON.stringify(x).toLowerCase().includes(q.value.toLowerCase())).map(x=>'<div class="item"><strong>'+esc(x.configuration)+' · '+esc(x.name)+'</strong>'+x.differences.map(d=>'<p><code>'+esc(d.kind)+'</code>：SDK <code>'+esc(value(d.sdk,d.kind))+'</code> → 导出 <code>'+esc(value(d.projection,d.kind))+'</code></p>').join('')+'<div class="path">SDK：'+esc(x.sdk.file||'')+':'+esc(x.sdk.line||'')+'<br>导出：'+esc(x.projection.file)+':'+esc(x.projection.line)+'</div></div>').join('')};q.oninput=c.onchange=render;render();</script></html>'''
    (output / 'report.html').write_text(page, encoding='utf-8')
    print(json.dumps({'configurations': len(summaries), 'all_symbols_verified': result['all_symbols_verified'], 'layout_examples': len(examples)}, ensure_ascii=False))
    return result


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--output', type=Path, default=Path('Z:/sdk-deep-audit-20261105'))
    summarize(parser.parse_args().output)
