#!/usr/bin/env python

# ===================================================
# THIS SOFTWARE IS BUILD TO WORK BEST WITH THE
# "excel-parser" NODE PACKAGE.
# ===================================================

__author__ = "Shekhar R. Thawali <shekhar.thawali@vxtindia.com>"
__version__ = "0.1.2"
__license__ = "GPL-2+"

import csv, datetime, zipfile, sys
import codecs, cStringIO, os, json
import xml.parsers.expat
from xml.dom import minidom

try:
  from argparse import ArgumentParser
except ImportError:
  print("\n******************************************************************")
  print("    argparse is required to run this script.")
  print("    It is available here: https://pypi.python.org/pypi/argparse")
  print("****************************************************************\n")
  sys.exit(1)
try:
  import xlrd
except ImportError:
  print("\n******************************************************************")
  print("    xlrd is required to run this script.")
  print("    It is available here: http://pypi.python.org/pypi/xlrd")
  print("****************************************************************\n")
  sys.exit(1)

FORMATS = {
  'general' : 'float',
  '0' : 'float',
  '0.00' : 'float',
  '#,##0' : 'float',
  '#,##0.00' : 'float',
  '0%' : 'percentage',
  '0.00%' : 'percentage',
  '0.00e+00' : 'float',
  'mm-dd-yy' : 'date',
  'd-mmm-yy' : 'date',
  'd-mmm' : 'date',
  'mmm-yy' : 'date',
  'h:mm am/pm' : 'date',
  'h:mm:ss am/pm' : 'date',
  'h:mm' : 'time',
  'h:mm:ss' : 'time',
  'm/d/yy h:mm' : 'date',
  '#,##0 ;(#,##0)' : 'float',
  '#,##0 ;[red](#,##0)' : 'float',
  '#,##0.00;(#,##0.00)' : 'float',
  '#,##0.00;[red](#,##0.00)' : 'float',
  'mm:ss' : 'time',
  '[h]:mm:ss' : 'time',
  'mmss.0' : 'time',
  '##0.0e+0' : 'float',
  '@' : 'float',
  'yyyy\\-mm\\-dd' : 'date',
  'dd/mm/yy' : 'date',
  'hh:mm:ss' : 'time',
  "dd/mm/yy\\ hh:mm" : 'date',
  'dd/mm/yyyy hh:mm:ss' : 'date',
  'yy-mm-dd' : 'date',
  'd-mmm-yyyy' : 'date',
  'm/d/yy' : 'date',
  'm/d/yyyy' : 'date',
  'dd-mmm-yyyy' : 'date',
  'dd/mm/yyyy' : 'date',
  'mm/dd/yy hh:mm am/pm' : 'date',
  'mm/dd/yyyy hh:mm:ss' : 'date',
  'yyyy-mm-dd hh:mm:ss' : 'date',
}

STANDARD_FORMATS = {
  0 : 'general',
  1 : '0',
  2 : '0.00',
  3 : '#,##0',
  4 : '#,##0.00',
  9 : '0%',
  10 : '0.00%',
  11 : '0.00e+00',
  12 : '# ?/?',
  13 : '# ??/??',
  14 : 'mm-dd-yy',
  15 : 'd-mmm-yy',
  16 : 'd-mmm',
  17 : 'mmm-yy',
  18 : 'h:mm am/pm',
  19 : 'h:mm:ss am/pm',
  20 : 'h:mm',
  21 : 'h:mm:ss',
  22 : 'm/d/yy h:mm',
  37 : '#,##0 ;(#,##0)',
  38 : '#,##0 ;[red](#,##0)',
  39 : '#,##0.00;(#,##0.00)',
  40 : '#,##0.00;[red](#,##0.00)',
  45 : 'mm:ss',
  46 : '[h]:mm:ss',
  47 : 'mmss.0',
  48 : '##0.0e+0',
  49 : '@',
}

def convert2csv(inFile, outFile, sheetFlag, sheetId=0, sheetName=None):
  fileName, fileExtension = os.path.splitext(inFile)
  if fileExtension == '.xls':
    writer = csv.writer(outFile, quoting=csv.QUOTE_MINIMAL, delimiter=',', dialect="excel")
    workbook = xlrd.open_workbook(filename=inFile)
    convertXls(workbook, sheetFlag, sheetId, sheetName, writer)
  elif fileExtension == '.xlsx':
    writer = csv.writer(outFile, quoting=csv.QUOTE_MINIMAL, delimiter=',')
    ziphandle = zipfile.ZipFile(inFile)
    convertXlsx(ziphandle, sheetFlag, sheetId, sheetName, writer)
  else:
    print Exception("Invalid file format")
    sys.exit(1)

def convertXlsx(ziphandle, sheetFlag, sheetId, sheetName, writer):
  shared_strings = parse(ziphandle, SharedStrings, "xl/sharedStrings.xml")
  styles = parse(ziphandle, Styles, "xl/styles.xml")
  workbook = parse(ziphandle, Workbook, "xl/workbook.xml")

  try:
    shared_strings = parse(ziphandle, SharedStrings, "xl/sharedStrings.xml")
    styles = parse(ziphandle, Styles, "xl/styles.xml")
    workbook = parse(ziphandle, Workbook, "xl/workbook.xml")

    if sheetFlag:
      print json.JSONEncoder().encode(workbook.sheets)
    elif sheetId > 0 or sheetName != None:
      if sheetId > 0:
        field = 'id'
        value = sheetId
      elif sheetName != None:
        field = 'name'
        value = sheetName
      else:
        print Exception("Wrong selection")
        sys.exit(1)

      wSheet = None
      for sheet in workbook.sheets:
        if sheet[field] == value:
          wSheet = Sheet(workbook, shared_strings, styles, ziphandle.read("xl/worksheets/sheet%i.xml" %sheet['id']))
          break
      if not wSheet:
        print Exception("Sheet %s not found" %value)
        sys.exit(1)
      wSheet.to_csv(writer)
    else:
      for sheet in workbook.sheets:
        wSheet = Sheet(workbook, shared_strings, styles, ziphandle.read("xl/worksheets/sheet%i.xml" %sheet['id']))
      wSheet.to_csv(writer)
  finally:
    ziphandle.close()

def convertXls(workbook, sheetFlag, sheetId, sheetName, writer):
  worksheets = []
  sheetnames = workbook.sheet_names()
  wSheet = None

  if sheetFlag:
    for index, sheet in enumerate(sheetnames):
      worksheets.append({"name": sheet, "id": index+1})
    print json.JSONEncoder().encode(worksheets)
  elif sheetId > 0 or sheetName != None:
    if sheetId > 0:
      sheetId = sheetId-1
      try:
        wSheet = workbook.sheet_by_index(sheetId)
      except Exception as ex:
        print Exception("Sheet %s not found" %sheetId)
    elif sheetName != None:
      try:
        wSheet = workbook.sheet_by_name(sheetName)
      except Exception as ex:
        print Exception("Sheet '%s' not found" %sheetName)

    if not wSheet:
      print Exception("Sheet %s not found" %sheetName)
      sys.exit(1)

    wSheet = XSheet(wSheet, writer)
    wSheet.to_csv()
  else:
    for sheet in sheetnames:
      wSheet = workbook.sheet_by_name(sheet)
    wSheet = XSheet(wSheet, writer)
    wSheet.to_csv()

def parse(ziphandle, klass, filename):
  instance = klass()
  if filename in ziphandle.namelist():
    instance.parse(ziphandle.read(filename))
  return instance

class Workbook:
  def __init__(self):
    self.sheets = []
    self.date1904 = False

  def parse(self, data):
    workbookDoc = minidom.parseString(data)
    if len(workbookDoc.firstChild.getElementsByTagName("fileVersion")) == 0:
      self.appName = 'unknown'
    else:
      self.appName = workbookDoc.firstChild.getElementsByTagName("fileVersion")[0]._attrs['appName'].value
    try:
      self.date1904 = workbookDoc.firstChild.getElementsByTagName("workbookPr")[0]._attrs['date1904'].value.lower().strip() != "false"
    except Exception as ex:
      pass

    sheets = workbookDoc.firstChild.getElementsByTagName("sheets")[0]
    for sheetNode in sheets.getElementsByTagName("sheet"):
      attrs = sheetNode._attrs
      name = attrs["name"].value
      if self.appName == 'xl':
        if attrs.has_key('r:id'): id = int(attrs["r:id"].value[3:])
        else: id = int(attrs['sheetId'].value)
      else:
        if attrs.has_key('sheetId'): id = int(attrs["sheetId"].value)
        else: id = int(attrs['r:id'].value[3:])
      self.sheets.append({'name': name, 'id': id})

class Styles:
  def __init__(self):
    self.numFmts = {}
    self.cellXfs = []

  def parse(self, data):
    styles = minidom.parseString(data).firstChild
    # numFmts
    numFmtsElement = styles.getElementsByTagName("numFmts")
    if len(numFmtsElement) == 1:
      for numFmt in numFmtsElement[0].childNodes:
        numFmtId = int(numFmt._attrs['numFmtId'].value)
        formatCode = numFmt._attrs['formatCode'].value.lower().replace('\\', '')
        self.numFmts[numFmtId] = formatCode
    # cellXfs
    cellXfsElement = styles.getElementsByTagName("cellXfs")
    if len(cellXfsElement) == 1:
      for cellXfs in cellXfsElement[0].childNodes:
        if (cellXfs.nodeName != "xf"):
          continue
        numFmtId = int(cellXfs._attrs['numFmtId'].value)
        self.cellXfs.append(numFmtId)

class SharedStrings:
  def __init__(self):
    self.parser = None
    self.strings = []
    self.si = False
    self.t = False
    self.rPh = False
    self.value = ""

  def parse(self, data):
    self.parser = xml.parsers.expat.ParserCreate()
    self.parser.CharacterDataHandler = self.handleCharData
    self.parser.StartElementHandler = self.handleStartElement
    self.parser.EndElementHandler = self.handleEndElement
    self.parser.Parse(data)

  def handleCharData(self, data):
    if self.t:
      self.value+= data

  def handleStartElement(self, name, attrs):
    if name == 'si':
      self.si = True
      self.value = ""
    elif name == 't' and self.rPh:
      self.t = False
    elif name == 't' and self.si:
      self.t = True
    elif name == 'rPh':
      self.rPh = True

  def handleEndElement(self, name):
    if name == 'si':
      self.si = False
      self.strings.append(self.value)
    elif name == 't':
      self.t = False
    elif name == 'rPh':
      self.rPh = False

class Sheet:
  def __init__(self, workbook, sharedString, styles, data):
    self.parser = None
    self.writer = None
    self.sharedString = None
    self.styles = None

    self.in_sheet = False
    self.in_row = False
    self.in_cell = False
    self.in_cell_value = False
    self.in_cell_formula = False

    self.columns = {}
    self.rowNum = None
    self.colType = None
    self.s_attr = None
    self.data = None

    self.dateformat = None
    self.skip_empty_lines = True

    self.data = data
    self.workbook = workbook
    self.sharedStrings = sharedString.strings
    self.styles = styles

  def to_csv(self, writer):
    self.writer = writer
    self.parser = xml.parsers.expat.ParserCreate()
    self.parser.CharacterDataHandler = self.handleCharData
    self.parser.StartElementHandler = self.handleStartElement
    self.parser.EndElementHandler = self.handleEndElement
    self.parser.Parse(self.data)

  def handleCharData(self, data):
    if self.in_cell_value:
      self.data = data # default value
      if self.colType == "s": # shared string
        self.data = self.sharedStrings[int(data)]
      elif self.colType == "b": # boolean
        self.data = (int(data) == 1 and "TRUE") or (int(data) == 0 and "FALSE") or data
      elif self.s_attr:
        s = int(self.s_attr)

        # get cell format
        format = None
        xfs_numfmt = self.styles.cellXfs[s]
        if self.styles.numFmts.has_key(xfs_numfmt):
          format = self.styles.numFmts[xfs_numfmt]
        elif STANDARD_FORMATS.has_key(xfs_numfmt):
          format = STANDARD_FORMATS[xfs_numfmt]
        # get format type
        if format and FORMATS.has_key(format):
          format_type = FORMATS[format]

          if format_type == 'date': # date/time
            try:
              if self.workbook.date1904:
                date = datetime.datetime(1904, 01, 01) + datetime.timedelta(float(data))
              else:
                date = datetime.datetime(1899, 12, 30) + datetime.timedelta(float(data))
              if self.dateformat:
                # str(dateformat) - python2.5 bug, see: http://bugs.python.org/issue2782
                self.data = date.strftime(str(self.dateformat))
              else:
                dateformat = format.replace("yyyy", "%Y").replace("yy", "%y"). \
                  replace("hh:mm", "%H:%M").replace("h", "%H").replace("%H%H", "%H").replace("ss", "%S"). \
                  replace("d", "%e").replace("%e%e", "%d"). \
                  replace("mmmm", "%B").replace("mmm", "%b").replace(":mm", ":%M").replace("m", "%m").replace("%m%m", "%m"). \
                  replace("am/pm", "%p")
                self.data = date.strftime(str(dateformat)).strip()
            except (ValueError, OverflowError):
              # invalid date format
              self.data = data
          elif format_type == 'time': # time
            self.data = str(float(data) * 24*60*60)

  def handleStartElement(self, name, attrs):
    if self.in_row and name == 'c':
      self.colType = attrs.get("t")
      self.s_attr = attrs.get("s")
      cellId = attrs.get("r")
      if cellId:
        self.colNum = cellId[:len(cellId)-len(self.rowNum)]
        self.colIndex = 0
      else:
        self.colIndex+= 1
      #self.formula = None
      self.data = ""
      self.in_cell = True
    elif self.in_cell and name == 'v':
      self.in_cell_value = True
    #elif self.in_cell and name == 'f':
    #  self.in_cell_formula = True
    elif self.in_sheet and name == 'row' and attrs.has_key('r'):
      self.rowNum = attrs['r']
      self.in_row = True
      self.columns = {}
      self.spans = None
      if attrs.has_key('spans'):
        self.spans = [int(i) for i in attrs['spans'].split(":")]
    elif name == 'sheetData':
      self.in_sheet = True

  def handleEndElement(self, name):
    if self.in_cell and name == 'v':
      self.in_cell_value = False
    #elif self.in_cell and name == 'f':
    #  self.in_cell_formula = False
    elif self.in_cell and name == 'c':
      t = 0
      for i in self.colNum: t = t*26 + ord(i) - 64
      self.columns[t - 1 + self.colIndex] = self.data
      self.in_cell = False
    if self.in_row and name == 'row':
      if len(self.columns.keys()) > 0:
        d = [""] * (max(self.columns.keys()) + 1)
        for k in self.columns.keys():
          d[k] = self.columns[k].encode("utf-8")
        if self.spans:
          l = self.spans[0] + self.spans[1] - 1
          if len(d) < l:
            d+= (l - len(d)) * ['']
        # write line to csv
        if not self.skip_empty_lines or d.count('') != len(d):
          self.writer.writerow(d)
      self.in_row = False
    elif self.in_sheet and name == 'sheetData':
      self.in_sheet = False

class XSheet:
  def __init__(self, wSheet, writer):
    self.wSheet = None
    self.writer = None
    self.rows = []
    self.queue = cStringIO.StringIO()

    self.wSheet = wSheet
    self.writer = writer
    self.convert()

  def convert(self):
    if self.wSheet:
      num_rows = self.wSheet.nrows - 1
      num_cells = self.wSheet.ncols - 1
      emptyType = set([xlrd.XL_CELL_BLANK, xlrd.XL_CELL_EMPTY]);
      curr_row = -1
      while curr_row < num_rows:
        curr_row += 1
        liste = []
        row = self.wSheet.row(curr_row)
        if not all(x in emptyType for x in row):
          curr_cell = -1
          while curr_cell < num_cells:
            curr_cell += 1
            cell_type = self.wSheet.cell_type(curr_row, curr_cell)
            cell_value = self.wSheet.cell_value(curr_row, curr_cell)

            if cell_type == xlrd.XL_CELL_BOOLEAN:
              try:
                liste.append(str(int(cell_value)))
              except (ValueError, OverflowError):
                liste.append(cell_value)
            elif cell_type == xlrd.XL_CELL_NUMBER:
              liste.append(str(cell_value))
            elif cell_type == xlrd.XL_CELL_DATE:
              try:
                cell_value = datetime.datetime(1899, 12, 30) + datetime.timedelta(float(cell_value))
                format = "d-mmm-yy";
                dateformat = format.replace("yyyy", "%Y").replace("yy", "%y"). \
                  replace("hh:mm", "%H:%M").replace("h", "%H").replace("%H%H", "%H").replace("ss", "%S"). \
                  replace("d", "%e").replace("%e%e", "%d"). \
                  replace("mmmm", "%B").replace("mmm", "%b").replace(":mm", ":%M").replace("m", "%m").replace("%m%m", "%m"). \
                  replace("am/pm", "%p")
                cell_value = cell_value.strftime(str(dateformat)).strip()
              except (ValueError, OverflowError):
                cell_value = cell_value
              liste.append(str(cell_value));
            elif cell_type == xlrd.XL_CELL_TEXT:
              cell_value = cell_value.encode("UTF-8", "strict")
              liste.append(cell_value)
            else:
              liste.append(cell_value)
        self.rows.append(liste)

  def to_csv(self):
    if self.rows:
      for row in self.rows:
        self.writer.writerow(row)

if __name__ == "__main__":
  parser = ArgumentParser(usage = "%%prog [options] infile [outfile]")
  parser.add_argument('-x', "--infile", dest="inFile", required = True, help="filename of the source spreadsheet");
  parser.add_argument('-c', "--outfile", dest="outFile", help="filename to save the generated csv file")
  parser.add_argument('-n', "--sheetnumber", dest="sheetId", default=0, type=int, help="sheet no to convert (0 for all sheets)")
  parser.add_argument("-w", "--sheetname", dest="sheetName", type=str, help="sheet name to convert", default=None);
  parser.add_argument("-W", "--worksheets", dest="sheetFlag", help="list all worksheets inside specified spreadsheet", action="store_true");

  args = parser.parse_args()
  if not len(sys.argv) > 1:
    parser.print_help()
  else:
    if args.sheetFlag:
      convert2csv(args.inFile, sys.stdout, args.sheetFlag)
    else:
      if args.inFile:
        if args.outFile:
          outfile = open(args.outFile, 'w+b');
        else:
          outfile = open('convert.csv', 'w+b');

        convert2csv(args.inFile, outfile, args.sheetFlag, args.sheetId, args.sheetName)
        outfile.close()
      else:
        parser.error("File is required")
