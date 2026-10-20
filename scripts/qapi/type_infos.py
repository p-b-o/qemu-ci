"""
QAPI type info generator

SPDX-License-Identifier: GPL-2.0-or-later
"""

from typing import List, Optional

from .common import c_name, mcgen
from .gen import QAPISchemaModularCVisitor, ifcontext
from .schema import (
    QAPISchema,
    QAPISchemaAlternatives,
    QAPISchemaBranches,
    QAPISchemaEnumMember,
    QAPISchemaFeature,
    QAPISchemaIfCond,
    QAPISchemaObjectType,
    QAPISchemaObjectTypeMember,
    QAPISchemaType,
)
from .source import QAPISourceInfo


class QAPISchemaGenTypeInfoVisitor(QAPISchemaModularCVisitor):

    def __init__(self, prefix: str, unmask: bool):
        super().__init__(
            prefix, 'qapi-type-infos',
            ' * Schema-defined QAPI type info',
            ' * Built-in QAPI type info', __doc__)
        self._unmask = unmask
        self._schema: Optional[QAPISchema] = None

    def visit_begin(self, schema: QAPISchema) -> None:
        super().visit_begin(schema)
        self._schema = schema

    def _begin_builtin_module(self) -> None:
        self._genc.preamble_add(mcgen('''
#include "qemu/osdep.h"
#include "qapi/qapi-builtin-types.h"
#include "qapi/qapi-builtin-type-infos.h"
'''))
        self._genh.preamble_add(mcgen('''
#include "qapi/qapi-type-info.h"
'''))

    def _begin_user_module(self, name: str) -> None:
        type_infos = self._module_basename('qapi-type-infos', name)
        types = self._module_basename('qapi-types', name)
        self._genc.preamble_add(mcgen('''
#include "qemu/osdep.h"
#include "%(types)s.h"
#include "%(type_infos)s.h"
''',
                                      types=types,
                                      type_infos=type_infos))
        self._genh.preamble_add(mcgen('''
#include "qapi/qapi-builtin-type-infos.h"
'''))

    def _has_list(self, name: str) -> bool:
        assert self._schema is not None
        return self._schema.lookup_type(name + 'List') is not None

    def _masked_name(self, name: str) -> str:
        assert self._schema is not None
        return self._schema.definition_introspection_name(name, self._unmask)

    def _gen_type_info(self, name: str,
                       ifcond: Optional[QAPISchemaIfCond] = None,
                       with_lookup: bool = False,
                       with_list: bool = False) -> None:
        c_id = c_name(name + '_type_info')
        masked_name_str = '"%s"' % self._masked_name(name)
        with ifcontext(ifcond or QAPISchemaIfCond(),
                       self._genh, self._genc):
            self._genh.add(mcgen('''

extern const QAPITypeInfo %(c_id)s;
''',
                                 c_id=c_id))
            self._genc.add(mcgen('''

const QAPITypeInfo %(c_id)s = {
    .name = "%(name)s",
    .masked_name = %(masked_name)s,
''',
                                 c_id=c_id, name=name,
                                 masked_name=masked_name_str))
            if with_lookup:
                self._genc.add(mcgen('''
    .lookup = &%(c_name)s_lookup,
''',
                                     c_name=c_name(name)))
            if with_list:
                self._genc.add(mcgen('''
    .list = &%(list_id)s,
''',
                                     list_id=c_name(name + 'List_type_info')))
            self._genc.add(mcgen('''
};
'''))

    def visit_builtin_type(self,
                           name: str,
                           info: Optional[QAPISourceInfo],
                           json_type: str) -> None:
        self._gen_type_info(name, with_list=self._has_list(name))

    def visit_enum_type(self,
                        name: str,
                        info: Optional[QAPISourceInfo],
                        ifcond: QAPISchemaIfCond,
                        features: List[QAPISchemaFeature],
                        members: List[QAPISchemaEnumMember],
                        prefix: Optional[str]) -> None:
        self._gen_type_info(name, ifcond,
                            with_lookup=True,
                            with_list=self._has_list(name))

    def visit_array_type(self,
                         name: str,
                         info: Optional[QAPISourceInfo],
                         ifcond: QAPISchemaIfCond,
                         element_type: QAPISchemaType) -> None:
        self._gen_type_info(name, ifcond)

    def visit_object_type(self,
                          name: str,
                          info: Optional[QAPISourceInfo],
                          ifcond: QAPISchemaIfCond,
                          features: List[QAPISchemaFeature],
                          base: Optional[QAPISchemaObjectType],
                          members: List[QAPISchemaObjectTypeMember],
                          branches: Optional[QAPISchemaBranches]) -> None:
        assert self._schema is not None
        typ = self._schema.lookup_type(name)
        assert typ
        if typ.is_implicit():
            return
        self._gen_type_info(name, ifcond,
                            with_list=self._has_list(name))

    def visit_alternate_type(self,
                             name: str,
                             info: Optional[QAPISourceInfo],
                             ifcond: QAPISchemaIfCond,
                             features: List[QAPISchemaFeature],
                             alternatives: QAPISchemaAlternatives) -> None:
        self._gen_type_info(name, ifcond,
                            with_list=self._has_list(name))


def gen_type_infos(schema: QAPISchema,
                   output_dir: str,
                   prefix: str,
                   opt_builtins: bool,
                   opt_unmask: bool) -> None:
    vis = QAPISchemaGenTypeInfoVisitor(prefix, opt_unmask)
    schema.visit(vis)
    vis.write(output_dir, opt_builtins)
